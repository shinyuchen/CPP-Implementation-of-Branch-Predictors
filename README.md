# Branch Predictors: TAGE Predictor Implementation

This repository contains a C++ implementation of branch predictors, focusing on the TAGE predictor, a state-of-the-art branch predictor introduced by André Seznec and colleagues. This implementation is designed to work seamlessly with the CBP-1 (Championship Branch Prediction) framework and datasets, providing a practical demonstration of the TAGE predictor's performance across different benchmark categories.

---

## Overview

### What is a Branch Predictor?
Branch prediction is a critical technique in modern processors that aims to reduce the performance penalties associated with control flow changes in programs. By predicting the outcome of conditional branches, processors can maintain high instruction throughput. Effective branch predictors, such as TAGE, play a crucial role in achieving low misprediction rates and high performance.

### About the TAGE Predictor
The TAGE (TAgged GEometric history length) predictor is a highly sophisticated branch predictor. It employs multiple tables indexed by different history lengths to capture both short- and long-term correlations. Introduced by André Seznec et al., TAGE has been a benchmark in branch prediction research:

- **Reference Papers:**
  - [A PPM-like, tagged-based branch predictors](https://jilp.org/vol7/v7paper10.pdf) 
  - [The TAGE Predictor](https://jilp.org/vol8/v8paper1.pdf)
  - [The CBP-1 Competition](https://jilp.org/cbp/)

This implementation adheres to the principles outlined in these foundational papers.

---

## Features

- **Compatibility:** Fully runnable within the [CBP-1 framework](https://jilp.org/cbp/).
- **Configurable Parameters:** Adjust predictor parameters, including history lengths and table sizes, for experimental purposes.
- **Detailed Implementation:** Incorporates advanced features such as probabilistic allocation and efficient tag management.

---

## Results

The following table summarizes the performance of this TAGE predictor implementation across CBP-1 benchmark categories:

| Testcase       | Avg. MPKI (Mispredictions Per 1000 Instructions) |
|----------------|--------------------------------------------------|
| FP (Floating Point) Average     | 0.5222                           |
| INT (Integer) Average           | 3.6414                           |
| MM (Memory Management) Average  | 4.9992                           |
| SERV (Server) Average           | 6.1432                           |

---

## Usage

### Prerequisites
- **CBP-1 Framework:** Download from the [official website](https://jilp.org/cbp/).

### Building the Project
1. Ensure the CBP-1 framework and dataset are available in the working directory.
2. Clone the repository:
   ```bash
   git clone https://github.com/shinyuchen/CPP-Implementation-of-Branch-Predictors
   ```
2. overwrite the predictor.h in the CBP-1 directory with ./tage/predictor.h:
   ```bash
   cp CPP-Implementation-of-Branch-Predictors/tage/predictor.h .
   ```
3. follow the instructions in CBP-1 framework to execute
   ```bash
   make
   ./predictor <CBP-1-dataset>
   ```

---

## Future Work
- **Optimizations:** Enhance predictor accuracy for SERV workloads by fine-tuning geometry lengths and employ more complex hash functions.
- **Extensions:** Explore hybrid predictors by combining TAGE with other techniques like perceptrons.

---

## References

1. André Seznec et al., "The TAGE Predictor," [JILP, 2005](https://jilp.org/vol8/v8paper1.pdf).
2. Championship Branch Prediction (CBP-1), [JILP, 2004](https://jilp.org/cbp/).

---



