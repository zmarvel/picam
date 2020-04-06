package main

import (
	"fmt"
	"os"
	"log"
	"github.com/astrogo/fitsio"
	"image"
	//"image/color"
	"image/png"
)

func printHelp() {
	fmt.Println("Usage: png2fits output [inputs]")
	fmt.Println("Convert a stack of PNG images to a FITS file.")
}

func main() {
	args := os.Args[1:]
	if len(args) < 1 {
		printHelp()
		os.Exit(1)
	}

	outputPath := args[0]

	inputPaths := args[1:]

	f, err := os.Create(outputPath)
	if err != nil {
		log.Fatalf("Failed to open output file %v: %+v", err)
	}
	defer f.Close()

	fits, err := fitsio.Create(f)
	if err != nil {
		log.Fatalf("Failed to create FITS file: %+v", err)
	}
	defer fits.Close()

	bitpix := 8


	for _, inputPath := range inputPaths {
		inputFile, err := os.Open(inputPath)
		if err != nil {
			log.Fatalf("Failed to open input file %v: %+v", inputPath, err)
		}
		defer inputFile.Close()

		// imgConfig, err := png.DecodeConfig(inputFile)
		// fmt.Printf("Config %+v", imgConfig);

		img, err := png.Decode(inputFile)
		if err != nil {
			log.Fatalf("Failed to decode input file %v: %+v", inputPath, err)
		}

		var depthBytes int
		var pixels []uint8
		//model := img.ColorModel()
		switch typedImg := img.(type) {
		case *image.RGBA:
			depthBytes = 4
			pixels = typedImg.Pix
		case *image.NRGBA:
			depthBytes = 4
			pixels = typedImg.Pix
		default:
			log.Fatalf("Unsupported input file format: %T", img)
		}
		width := img.Bounds().Max.X
		height := img.Bounds().Max.Y
		axes := []int{width, height, depthBytes}

		fitsImg := fitsio.NewImage(bitpix, axes)
		defer fitsImg.Close()

		err = fitsImg.Write(pixels)
		if err != nil {
			log.Fatalf("Failed to write input image to FITS image: %+v", err)
		}

		err = fits.Write(fitsImg)
		if err != nil {
			log.Fatalf("Failed to write FITS image to FITS file: %+v", err)
		}
	}
}
