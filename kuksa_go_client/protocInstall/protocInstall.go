package main

import (
	"archive/zip"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"runtime"
)

const (
	Name        = "protoc"
	ZipFileName = Name + ".zip"
	OsMac = "darwin"
	OsWindows = "windows"
	OsLinux = "linux"
)

func DownloadPackage(url string) error {
	resp, err := http.Get(url)
	if err != nil {
		return err
	}
    defer resp.Body.Close()

	out, err := os.Create(ZipFileName)
    if err != nil {
		return err
	}
    defer out.Close()

	if _, err := io.Copy(out, resp.Body); err != nil {
		return err
	}
	return nil
}

func UnzipSource(source, destination string) error {
    reader, err := zip.OpenReader(source)
    if err != nil {
        return err
    }
    defer reader.Close()

    destination, err = filepath.Abs(destination)
    if err != nil {
        return err
    }

    for _, f := range reader.File {
		filePath := filepath.Join(destination, f.Name)

		if f.FileInfo().IsDir() {
			os.MkdirAll(filePath, os.ModePerm)
			continue
		} else {
			fmt.Println("unziping files to", filePath)
			
			out, err := os.Create(filePath)
			if err != nil {
				return err
			}
			defer out.Close()

			fileInArchive, err := f.Open()
			if err != nil {
				return err
			}

			if _, err := io.Copy(out, fileInArchive); err != nil {
				return err
			}
		}
    }

    return nil
}

var url = map[string]string{
	"linux_32":   "https://github.com/protocolbuffers/protobuf/releases/download/v3.19.4/protoc-3.19.4-linux-x86_32.zip",
	"linux_64":   "https://github.com/protocolbuffers/protobuf/releases/download/v3.19.4/protoc-3.19.4-linux-x86_64.zip",
	"darwin":     "https://github.com/protocolbuffers/protobuf/releases/download/v3.19.4/protoc-3.19.4-osx-x86_64.zip",
	"windows_32": "https://github.com/protocolbuffers/protobuf/releases/download/v3.19.4/protoc-3.19.4-win32.zip",
	"windows_64": "https://github.com/protocolbuffers/protobuf/releases/download/v3.19.4/protoc-3.19.4-win64.zip",
}

type NotFoundError struct{}

func (m *NotFoundError) Error() string {
	return "HOME was not found"
}

func Install() error {
	goos := runtime.GOOS
	bit := 32 << (^uint(0) >> 63)
	var downloadUrl string
	switch goos {
		case OsMac:
			downloadUrl = url[OsMac]
		case OsWindows:
			downloadUrl = url[fmt.Sprintf("%s_%d", OsWindows, bit)]
		case OsLinux:
			downloadUrl = url[fmt.Sprintf("%s_%d", OsLinux, bit)]
		default:
			return fmt.Errorf("unsupport OS: %q", goos)
	}

	print("Downloading protobuf compiler...\n")
	err := DownloadPackage(downloadUrl)
	if err != nil {
		return err
	}

	var home string
	_, found := os.LookupEnv("HOME")
    if found {
		home = os.Getenv("HOME")
    } else {
        return &NotFoundError{}
    }

	dest := filepath.Join(home, Name)

	print("Unziping source...")
	return UnzipSource(ZipFileName, dest)
}

func Exists() bool {
	var root string
	_, found := os.LookupEnv("HOME")
    if found {
		root = os.Getenv("HOME")
    } else {
        return false
    }
	protocPath := filepath.Join(root, Name)
	if _, err := os.Stat(protocPath); os.IsNotExist(err) {
		return false
	}

	return true
}

func ProtoExists() {
	if _, err := os.Stat("proto"); os.IsNotExist(err) {
		print("no directory proto exists. Creating...\n")
		os.MkdirAll("proto", os.ModePerm)
	}
}

func main() {
	if !Exists(){
		print("No installed protobuf compiler found! Installing... \n")
		err := Install()
		if err != nil {
			panic(err)
		}
	}
	ProtoExists()
	print("All done!\n")
}