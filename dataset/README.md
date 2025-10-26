# About the Datasets

We conduct our experiments using four datasets, including three public real-world datasets and one synthetic dataset.  

For each public dataset, we provide a small demo file in this repository to help verify that the code runs correctly.  Meanwhile, we also provide official download links for users to obtain the full datasets for extended experiments.

For the synthetic dataset, we include the data generation script used in our experiments, allowing users to reproduce the same dataset or test algorithm performance under different distributions.

---

## CAIDA Anonymized Internet Traces (2019)

- **Description**: This dataset contains anonymized Internet traffic traces collected by **CAIDA** in 2019. Each packet is identified by its source and destination IP addresses.
- **Demo File**: A small demo trace (txt format) is included in this repository for testing. We download the raw CAIDA data, parse the raw traffic and convert it into plain text (txt) format.
- **Full Dataset**: Available upon request from the official CAIDA website:  
  https://catalog.caida.org/dataset/passive_2019_pcap

---

## MAWI Traffic Traces (2024)

- **Description**: The MAWI dataset is maintained by [the MAWI Working Group](http://www.wide.ad.jp/project/wg/mawi.html) of [the WIDE Project](http://www.wide.ad.jp/). It provides daily backbone traffic traces captured at the WIDE–ISP transit link, widely used in tasks such as heavy hitter identification.
- **Demo File**: A small demo file (in CSV format) is provided, parsed from the original MAWI traces.
- **Full Dataset**: Can be downloaded from the MAWI archive:  
  https://mawi.wide.ad.jp/mawi/

---

## Frequent Itemset Mining Dataset

- **Description**: This is the public datasets from the Frequent Itemset Mining Dataset repository. 
- **Demo File**: we provide the "kosarak.dat" dataset as a demo, which contains (anonymized) click-stream data of a hungarian on-line news portal.
- **Full Dataset**: Can be obtained from the open data archive: http://fimi.uantwerpen.be/data/

---

## Synthetic Dataset

- **Description**:  We generate a synthetic dataset based on a Zipf distribution.  It allows evaluation of algorithm performance under different skew parameters.

- **Generation Script**:  
  The script used to generate this dataset is included in the repository (`genSyntheticDataset.py`).  
  Users can adjust parameters to create datasets with different statistical characteristics.

---

> ⚠️ **Note**: The demo datasets provided in this repository are for research and code testing only. Please comply with the official dataset usage policies and cite their sources when using the full datasets.
