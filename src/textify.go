package main

import (
	"fmt"
	"image"
	"image/jpeg"
	"log"
	"os"
	"os/exec"
	"strings"

	"github.com/nathan-fiscaletti/consolesize-go"
	"github.com/nfnt/resize"
)

func FatalIfTrue(e error) {
	if e != nil {
		log.Fatal(e)
	}
}

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

func IsValidAndType(fileName string) string {
	validImageExts := []string{".jpg"}
	validVideoExts := []string{".mp4"}

	fileExtIndex := strings.Index(fileName, ".")
	fileExt := fileName[fileExtIndex:]
	fmt.Printf("File extension: '%s'\n", fileExt)
	if IsStringInList(fileExt, validImageExts) {
		return "image"
	} else if IsStringInList(fileExt, validVideoExts) {
		return "video"
	} else {
		return "!valid"
	}
}

func PrintImage(filePath string, windowHeight int) {
	charmap := " -:;~+?#8$@"
	// Open the image
	image.RegisterFormat("jpeg", "jpg", jpeg.Decode, jpeg.DecodeConfig)
	file, err := os.Open(filePath)
	FatalIfTrue(err)
	defer file.Close()
	// Load the image into readable form
	img, _, err := image.Decode(file)
	FatalIfTrue(err)
	// Get the image's size (bounds)
	bounds := img.Bounds()
	originalWidth, originalHeight := bounds.Max.X, bounds.Max.Y
	fmt.Printf("Image size: %d x %d\n", originalWidth, originalHeight)
	// Get the width, adjusted to the terminal's size
	// (*2 because 1 char is double high than wide)
	newWidth := int(float32(originalWidth) / float32(originalHeight) * float32(windowHeight) * 2)
	fmt.Printf("New terminal size: %d x %d\n", newWidth, windowHeight)
	// Resize the original image to the terminal's size
	resizedImg := resize.Resize(uint(newWidth), uint(windowHeight), img, resize.NearestNeighbor)
	// Clear screen
	cmd := exec.Command("cmd", "/c", "cls")
	cmd.Stdout = os.Stdout
	cmd.Run()
	// Loop through every pixel and print the associated char with its luminescence
	for y := 0; y < windowHeight; y++ {
		for x := 0; x < newWidth; x++ {
			r, g, b, _ := resizedImg.At(x, y).RGBA()
			lum := CalcLuminescence(r, g, b)
			lum = Normalize(lum, 65535, len(charmap)-1)
			char := charmap[lum]
			fmt.Printf("%c", char)
		}
		fmt.Printf("\n")
	}
}

func CalcLuminescence(r, g, b uint32) int {
	return int((r + g + b) / 3)
}

func Clamp(value, min, max int) int {
	if value < min {
		return min
	} else if value > max {
		return max
	}
	return value
}

func Normalize(value, oldMax, newMax int) int {
	weight := float64(oldMax) / float64(newMax)
	return int(float64(value) / weight)
}

func main() {
	// Get the size (in characters) of the current window
	windowWidth, windowHeight := consolesize.GetConsoleSize()
	fmt.Printf("Terminal size: %d x %d\n", windowWidth, windowHeight)
	// Get the input file's path
	pwd, err := os.Getwd()
	FatalIfTrue(err)
	inputName := GetInputMedia()
	inputPath := pwd + "\\res\\" + inputName
	fmt.Printf("Chosen file path: '%s'\n", inputPath)
	// Varify that the file is readable (valid), and get its type (image/video)
	inputType := IsValidAndType(inputName)
	if inputType == "!valid" {
		fmt.Printf("%s\n", "The provided input type/extension is not supported, sorry!")
	} else if inputType == "image" {
		PrintImage(inputPath, windowHeight)
	} else if inputType == "video" {
		fmt.Printf("%s\n", "File is video")
	} else {
		fmt.Printf("%s\n", "IsValidAndType gave erroneous response. Issue Unknown.")
	}
}
