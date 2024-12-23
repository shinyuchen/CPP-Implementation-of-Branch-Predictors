/* Author: Jared Stark;   Created: Tue Jul 27 13:39:15 PDT 2004
 * Description: This file defines a gshare branch predictor.
*/

#ifndef PREDICTOR_H_SEEN
#define PREDICTOR_H_SEEN

#include <cstddef>
#include <inttypes.h>
#include <vector>
#include "op_state.h"   // defines op_state_c (architectural state) class 
#include "tread.h"      // defines branch_record_c class

// assume hardware budget is 64 Kbits (2^16 bits)
class PREDICTOR
{
  private:
    // 8 component configuration, which follows the configurations specified in the paper (https://jilp.org/vol8/v8paper1.pdf)
    // hardware = bimodal_entries * pred_bits = 2 ^ 13 = 8192 bits
    static const int bimodal_bits = 12; 
    static const int bimodal_entries = 1 << bimodal_bits;
    static const int bimodal_pred_bits = 2; 
    int8_t *bimodal;

    // 7 tagged bank predictors
    // hardware = (NUM_BANK) * (entries * (3 + 2 + tag_bits)) = 57344
    static const int NUM_BANKS = 4;
    static const int ENTRY_BITS = 10;
    static const int TAG_BITS = 9;
    static const int MIN_GEOMETRY_LEN = 5;
    static const int MAX_GEOMETRY_LEN = 128;
    static const int u_bits = 2;
    static const int pred_bits = 3;
    int refresh_counter;
    bool refresh_MSB;
    int prob;
    int total_prob;

    uint8_t *tage_ghr;

    struct bank_entry{
        uint16_t tag;
        uint8_t u;
        int8_t pred;
    };

    struct bank{
        bank_entry entry[1 << ENTRY_BITS];
        int geometry_len;
        uint8_t CSR[ENTRY_BITS];
        uint8_t CSR1[TAG_BITS];
        uint8_t CSR2[TAG_BITS-1];
        int16_t hash_index;
        int16_t hash_tag;
        bool hit;
        int allocate_prob;
    };

    int8_t provider_component_index;
    bool altpred, pred;

    bank tage_bank[NUM_BANKS];

  public:
    PREDICTOR(void) : bimodal(), tage_ghr(), tage_bank() {
  // init base predictor
        bimodal = (int8_t *)malloc(bimodal_entries * sizeof(int8_t));
        for(int i = 0; i < bimodal_entries; i++){
            bimodal[i] = 1; // init to weakly not taken
        }
        // init banks
        for(int i = 0; i < NUM_BANKS; i++){
            // init geometry length
            // IMPLEMENTATION DETAIL: need to add the type conversion to double to provide the correct power
            tage_bank[i].geometry_len = (int) (pow((double) (MAX_GEOMETRY_LEN / MIN_GEOMETRY_LEN), (double) i / (NUM_BANKS-1)) * MIN_GEOMETRY_LEN + 0.5);
            printf("geometry len %d = %d\n", i, tage_bank[i].geometry_len);
            // init CSR
            for(int j = 0; j < ENTRY_BITS; j++){
                tage_bank[i].CSR[j] = 0;
            }
            for(int j = 0; j < TAG_BITS; j++){
                tage_bank[i].CSR1[j] = 0; 
            }
            for(int j = 0; j < TAG_BITS-1; j++){
                tage_bank[i].CSR2[j] = 0;
            }
            // init bank_entry
            for(int j = 0; j < (1 << ENTRY_BITS); j++){
                tage_bank[i].entry[j].tag = 0;
                tage_bank[i].entry[j].u = 0;
                tage_bank[i].entry[j].pred = 4; // init to weakly taken
            }
        }
        // init ghr
        tage_ghr = (uint8_t *)malloc(MAX_GEOMETRY_LEN * sizeof(uint8_t));
        for(int i = 0; i < MAX_GEOMETRY_LEN; i++){
            tage_ghr[i] = 0;
        }

        // init refresh counter
        refresh_counter = 1;
        refresh_MSB = 1;

    }
    // uses compiler generated copy constructor
    // uses compiler generated destructor
    // uses compiler generated assignment operator

    // get_prediction() takes a branch record (br, branch_record_c is defined in
    // tread.h) and architectural state (os, op_state_c is defined op_state.h).
    // Your predictor should use this information to figure out what prediction it
    // wants to make.  Keep in mind you're only obligated to make predictions for
    // conditional branches.
    bool get_prediction(const branch_record_c* br, const op_state_c* os)
    // bool get_prediction(address_t pc)
        {
            provider_component_index = -1; // init to -1 for future checking if we have updated the value
            
            // first get the prediction of the bimodal predictor
            uint32_t pc = br->instruction_addr;
            int bimodal_index = pc & (bimodal_entries - 1);
            altpred = bimodal[bimodal_index] >> (bimodal_pred_bits - 1); // MSB of the saturating counter
            pred = altpred;

            // then get the predictions of the tagged banks
            for(int i = 0; i < NUM_BANKS; i++){
                // get the hashed bank entry index
                tage_bank[i].hash_index = 0;
                for(int j = 0; j < ENTRY_BITS; j++){
                    tage_bank[i].hash_index |= (tage_bank[i].CSR[j] ^ ((pc >> j) & 1)) << j;
                }
                // get the hashed tag for comparison
                tage_bank[i].hash_tag = 0;
                tage_bank[i].hash_tag |= tage_bank[i].CSR1[0] ^ (pc & 1);
                for(int j = 1; j < TAG_BITS; j++){
                tage_bank[i].hash_tag |= (tage_bank[i].CSR1[j] ^ tage_bank[i].CSR2[j-1] ^ ((pc >> j) & 1)) << j;
                }
                // check if the hashed tag and the bank tag is a hit
                tage_bank[i].hit = (tage_bank[i].hash_tag == tage_bank[i].entry[tage_bank[i].hash_index].tag);
                if(tage_bank[i].hit){
                    if(provider_component_index == -1){
                        provider_component_index = i;
                        pred = tage_bank[i].entry[tage_bank[i].hash_index].pred >> (pred_bits - 1);
                    }
                    else{
                        // make the previous prediction the altpred
                        altpred = pred;
                        provider_component_index = i;
                        pred = tage_bank[i].entry[tage_bank[i].hash_index].pred >> (pred_bits - 1);
                    }
                }
            }
            return pred;
        }

    // Update the predictor after a prediction has been made.  This should accept
    // the branch record (br) and architectural state (os), as well as a third
    // argument (taken) indicating whether or not the branch was taken.
    // void update_predictor(address_t pc, bool taken)
    void update_predictor(const branch_record_c* br, const op_state_c* os, bool taken)
        {
            uint32_t pc = br->instruction_addr;

            // update the bimodal predictor no matter it is the provider component or not
            int bimodal_index = pc & (bimodal_entries - 1);
            if(taken){
                if(bimodal[bimodal_index] != 3){
                    bimodal[bimodal_index] ++;
                }
            }
            else{
                if(bimodal[bimodal_index] != 0){
                    bimodal[bimodal_index] --;
                }
            }

            // update the tagged banks

            // update the u
            if(provider_component_index != -1){
                if(altpred != pred){
                    if(pred == taken){
                        if(tage_bank[provider_component_index].entry[tage_bank[provider_component_index].hash_index].u != 3){
                            tage_bank[provider_component_index].entry[tage_bank[provider_component_index].hash_index].u ++;
                        }
                    }
                    else{
                        if(tage_bank[provider_component_index].entry[tage_bank[provider_component_index].hash_index].u != 0){
                            tage_bank[provider_component_index].entry[tage_bank[provider_component_index].hash_index].u --;
                        }
                    }
                }
            }
            // refreshing u every 256K branches
            if(refresh_counter % (1 << 18) == 0){
                if(refresh_MSB){
                    for(int i = 0; i < NUM_BANKS; i++){
                        for(int j = 0; j < (1 << ENTRY_BITS); j++){
                            tage_bank[i].entry[j].u &= 0b01;
                        }
                    }
                    refresh_MSB = 0;
                }
                else{
                    for(int i = 0; i < NUM_BANKS; i++){
                        for(int j = 0; j < (1 << ENTRY_BITS); j++){
                            tage_bank[i].entry[j].u &= 0b10;
                        }
                    }
                    refresh_MSB = 1;
                }
            }

            // update the pred counter

            if(provider_component_index != -1){
                if(taken){
                    if(tage_bank[provider_component_index].entry[tage_bank[provider_component_index].hash_index].pred != 7){
                        tage_bank[provider_component_index].entry[tage_bank[provider_component_index].hash_index].pred ++;
                    }
                }
                else{
                    if(tage_bank[provider_component_index].entry[tage_bank[provider_component_index].hash_index].pred != 0){
                        tage_bank[provider_component_index].entry[tage_bank[provider_component_index].hash_index].pred --;
                    }
                }
            }

            // new entry allocation 
            prob = 1;
            total_prob = 0;
            int rand_num = rand();
            if(taken != pred){
                if(provider_component_index != NUM_BANKS-1){
                    // 1. get all the u counter of the bank entry from provider_component_index+1 to NUM_BANKS-1
                    // 2. if exists one or more entry with u = 0 -> allocate one among them with different probabilities
                    for(int i = NUM_BANKS-1; i > provider_component_index; i--){
                        if(tage_bank[i].entry[tage_bank[i].hash_index].u == 0){
                            total_prob += prob;
                            tage_bank[i].allocate_prob = total_prob; // the bank with shorter history has higher probability to be allocated
                            prob = prob * 2;
                        }
                    }

                    if(total_prob != 0){
                        rand_num = rand_num % total_prob;
                        for(int i = NUM_BANKS-1; i > provider_component_index; i--){
                            if(tage_bank[i].entry[tage_bank[i].hash_index].u == 0){
                                // init the entry
                                if(tage_bank[i].allocate_prob > rand_num){
                                    tage_bank[i].entry[tage_bank[i].hash_index].tag = tage_bank[i].hash_tag;
                                    tage_bank[i].entry[tage_bank[i].hash_index].pred = (taken) ? 4 : 3;
                                    tage_bank[i].entry[tage_bank[i].hash_index].u = 0;
                                    break;
                                }
                            }
                        }
                    }
                    // 3. if no entry is allocated, decrement all the u counters 
                    else{
                        for(int i = NUM_BANKS-1; i > provider_component_index; i--){
                            if(tage_bank[i].entry[tage_bank[i].hash_index].u != 0){
                                tage_bank[i].entry[tage_bank[i].hash_index].u --;
                            }
                        }
                    }
                }
            }

            // update the ghr
            for(int i = MAX_GEOMETRY_LEN-1; i > 0; i--){
                tage_ghr[i] = tage_ghr[i-1];
            }
            tage_ghr[0] = taken;

            // update the CSRs
            int temp;
            for(int i = 0; i < NUM_BANKS; i++){
                // CSR
                // 1. circular shift
                temp = tage_bank[i].CSR[ENTRY_BITS-1];
                for(int j = ENTRY_BITS-1; j > 0; j--){
                    tage_bank[i].CSR[j] = tage_bank[i].CSR[j-1];
                }
                tage_bank[i].CSR[0] = temp;
                // 2. xor the new history
                tage_bank[i].CSR[0] ^= taken;
                // 3. xor the outdated history
                tage_bank[i].CSR[tage_bank[i].geometry_len % ENTRY_BITS] ^= tage_ghr[tage_bank[i].geometry_len];
                
                // CSR1
                temp = tage_bank[i].CSR1[TAG_BITS-1];
                for(int j = TAG_BITS-1; j > 0; j--){
                    tage_bank[i].CSR1[j] = tage_bank[i].CSR1[j-1];
                }
                tage_bank[i].CSR1[0] = temp;
                tage_bank[i].CSR1[0] ^= taken;
                tage_bank[i].CSR1[tage_bank[i].geometry_len % TAG_BITS] ^= tage_ghr[tage_bank[i].geometry_len];

                // CSR2
                temp = tage_bank[i].CSR2[TAG_BITS-2];
                for(int j = TAG_BITS-2; j > 0; j--){
                    tage_bank[i].CSR2[j] = tage_bank[i].CSR2[j-1];
                }
                tage_bank[i].CSR2[0] = temp;
                tage_bank[i].CSR2[0] ^= taken;
                tage_bank[i].CSR2[tage_bank[i].geometry_len % (TAG_BITS - 1)] ^= tage_ghr[tage_bank[i].geometry_len];
            }
            refresh_counter ++;
        }
};

#endif // PREDICTOR_H_SEEN

