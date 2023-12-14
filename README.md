# File System Daemon for WAD Data Access

## Overview

This repository contains the source code for a file system daemon implemented in C, using the FUSE API. The purpose of this daemon is to provide access to data stored in a WAD-specific format. The file control blocks are processed and parsed through a user space library written in C++.

## Features

- **File System Daemon:** Utilizes the FUSE (Filesystem in Userspace) API to create a virtual file system for accessing WAD data.

- **WAD Format Support:** Handles data stored in the WAD (Where's All the Data) format, providing a seamless interface to access and manage the content.

- **C and C++ Integration:** The core file system functionality is implemented in C, while a user space library in C++ is employed for processing and parsing file control blocks.

## Requirements

- **FUSE:** Ensure that FUSE is installed on your system. You can find more information about FUSE at [https://github.com/libfuse/libfuse](https://github.com/libfuse/libfuse).

## Getting Started

1. **Clone the Repository:**

   ```bash
   git clone given repository
   cd your-repository

2. **Test Files to be added soon...**
