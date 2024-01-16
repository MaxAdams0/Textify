package main

import (
	"fmt"
	"image"
	"image/jpeg"
	"image/png"
	"log"
	"os"
	"strings"
	"time"

	"github.com/inancgumus/screen"
)

func main() {
	wW, wH := screen.Size()
	fmt.Printf("Window Size: %d, %d\n", wW, wH)

	// Get the input file's path
	pwd, err := os.Getwd()
	FatalIfTrue(err)
	inputPath := pwd + "\\res\\" + GetInputMedia()
	// Varify that the file is readable (valid), and get its type (image/sequence)
	inputExt := GetFileExt(inputPath)
	inputType := IsValidAndType(inputExt)
	// Set the encoding for the images
	if inputExt == "jpg" {
		image.RegisterFormat("jpeg", "jpg", jpeg.Decode, jpeg.DecodeConfig)
	} else if inputExt == "png" {
		image.RegisterFormat("png", "png", png.Decode, png.DecodeConfig)
	}
	// Do different actions for each type of input
	if inputType == "!valid" {
		fmt.Printf("%s\n", "The provided input type/extension is not supported, sorry!")
	} else if inputType == "image" {
		PrintImage(inputPath)
	} else if inputType == "sequence" {
		// Read all elements in the dir
		files, err := os.ReadDir(inputPath)
		FatalIfTrue(err)
		for _, f := range files {
			framePath := inputPath + "\\" + f.Name()
			frameTime := PrintImage(framePath)
			fps := int(1 / frameTime)
			fmt.Printf("File: %s | Frame Time: %f | FPS: %03d", f.Name(), frameTime, fps)
			// Sleep to time FPS (REALLY BIG NOTE, THIS WILL DESYNC BC OF PROCESSING TIME)
			//sleepDuration := time.Second / time.Duration(fps)
			//time.Sleep(sleepDuration)
		}
	} else {
		fmt.Printf("%s\n", "IsValidAndType gave erroneous response. Issue Unknown.")
	}
}

// ==================== APPLICATION SPECIFIC FUNCTIONS ============================================================================================================================

func GetInputMedia() string {
	// Read all elements in the dir "res"
	files, err := os.ReadDir("res")
	FatalIfTrue(err)
	// Print every element
	for i, f := range files {
		fmt.Printf("%d %s\n", i, f.Name())
	}
	// Get file selection from integer input
	var chosenFile int
	_, err = fmt.Scanf("%d", &chosenFile)
	FatalIfTrue(err)
	if chosenFile >= len(files) {
		log.Fatal("Selected file does not exist. Number provided is likely too large or small.")
	}
	fmt.Printf("Chosen file: #%d '%s'\n", chosenFile, files[chosenFile].Name())
	// Return the name of the selection
	return files[chosenFile].Name()
}

func IsStringInList(str string, list []string) bool {
	for _, elem := range list {
		if elem == str {
			return true
		}
	}
	return false
}

func GetFileExt(fileName string) string {
	fileExtIndex := strings.Index(fileName, ".")
	if fileExtIndex >= 0 {
		return fileName[fileExtIndex:]
	} else {
		return ""
	}
}

func IsValidAndType(fileExt string) string {
	validImageExts := []string{".jpg"}
	if IsStringInList(fileExt, validImageExts) {
		return "image"
	} else if fileExt == "" {
		return "sequence"
	} else {
		return "!valid"
	}
}

func PrintImage(filePath string) float32 {
	startTime := time.Now().UnixMicro()

	// Get the size (in characters) of the current window
	_, windowHeight := screen.Size()
	charmap := " .-=+*#%@" //[]rune{' ', '\u2591', '\u2592', '\u2593'}
	// Open the image
	file, err := os.Open(filePath)
	FatalIfTrue(err)
	defer file.Close()
	// Load the image into readable form
	img, _, err := image.Decode(file)
	FatalIfTrue(err)
	// Get the image's size (bounds)
	bounds := img.Bounds()
	originalWidth, originalHeight := bounds.Max.X, bounds.Max.Y
	// Get the width, adjusted to the terminal's size
	// (*2 because 1 char is double high than wide)
	newWidth := int(float32(originalWidth) / float32(originalHeight) * float32(windowHeight) * 2)
	widthRatio := float32(originalWidth) / float32(newWidth) // image pixels per char "pixel"
	heightRatio := float32(originalHeight) / float32(windowHeight)
	// Loop through every pixel and print the associated char with its luminescence
	var frame string
	for y := float32(0.0); y < float32(originalHeight); y += heightRatio {
		for x := float32(0.0); x < float32(originalWidth); x += widthRatio {
			r, g, b, _ := img.At(int(x), int(y)).RGBA()
			lum := CalcLumine(r, g, b)
			lum = Normalize(lum, 65535, len(charmap)-1)
			frame += string(charmap[lum])
		}
		frame += "\n"
	}
	fmt.Printf("\r%s", frame)
	endTime := time.Now().UnixMicro()
	frameTime := float32(endTime-startTime) / float32(1000000) // Elapsed time, Milliseconds -> Seconds
	return frameTime
}

// ==================== EXTRA UTILITY FUNCTIONS ===================================================================================================================================

func FatalIfTrue(e error) {
	if e != nil {
		log.Fatal(e)
	}
}

func CalcLumine(r, g, b uint32) int {
	return int((r + g + b) / 3)
}

func Normalize(value, oldMax, newMax int) int {
	weight := float32(oldMax) / float32(newMax)
	return int(float32(value) / weight)
}
