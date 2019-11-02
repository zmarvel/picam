# Design

## Overview

Requirements:

- Accept connections or field requests from at least one picam.
- Receive JPEG or PNG image data.
- Log received images.
- Extract interesting features from received images (processing stage).
- Log extracted features.


Nice to have:

- Also support receiving encoded video data.


## Components

Based on the requirements, the application can be divided into a few
obvious components.


### Receiving service

One component should accept connections from any number of picams. The
picams should uniquely identify themselves--can some unique ID be
extracted from the Pi processor?

Required image metadata:

- timestamp
- location
- camera facing? angle?
- shutter speed/integration time


### Recorder service

The received image data should be logged. Ideally the logs should be
structured like this:

```
<picam_0>
   |_ <day 1>
     |_ image_1
 	 |_ image_2
 	 |_ ...
 	 |_ image_n
   |_ ...
   |_ <day n>
 	 |_ ...
<picam_n>
   |_ ...
```

If the image timestamp and location are encoded in the image, the
image filename should just reflect its sequence number. Otherwise, the
timestamp and location should either be encoded in the filename or
(better) indexed in a database.

Although EXIF metadata can be included in JPEG images, it isn't
specified for PNGs. So the options for metadata are:

1. Custom file format.
2. Separate file to accompany each image.
3. In a database.

I think 2 is a cleaner, but equivalent, solution compared to 1. Since
the results of image processing are probably going to end up in a DB
anyway, let's go with 3.

We need a way to associate the original image files with the entry in
the DB. We can either store the path in the DB or maybe store the
timestamp in the filename.


### Processing service

The processing service should, at first, consider the location,
direction, and time of each image. It should identify interesting
features in the image (for example, stars, airplanes in long-exposure
shots, etc.). 

The results should end up in the DB.


## Implementation language

Only the processing service is locked in. I want to do it in C++ to
keep the option of custom OpenCL/CUDA kernels open.

Options for the receiver and recorder:

- Python
- Go
- Node.js
- Scala
- Java
- C++
- Clojure

Just for a first exposure, let's use Go.


## Future work
