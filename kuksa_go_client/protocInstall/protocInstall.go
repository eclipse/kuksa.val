package protocInstall

import (
	"archive/zip"
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"net/http"
)

const (
	Name        = "protoc"
	ZipFileName = Name + ".zip"
)

func downloadPackage(url string) {
    resp, _ := http.Get(url)
    defer resp.Body.Close()
    out, _ := os.Create(Name)
    defer out.Close()
    io.Copy(out, resp.Body)
}

func unzipSource(source, destination string) error {
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
        err := unzipFile(f, destination)
        if err != nil {
            return err
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

func Install(cacheDir string) (string, error) {
	goos := runtime.GOOS
	bit := 32 << (^uint(0) >> 63)
	var downloadUrl string
	switch goos {
		case vars.OsMac:
			downloadUrl = url[vars.OsMac]
		case vars.OsWindows:
			downloadUrl = url[fmt.Sprintf("%s_%d", vars.OsWindows, bit)]
		case vars.OsLinux:
			downloadUrl = url[fmt.Sprintf("%s_%d", vars.OsLinux, bit)]
		default:
			return "", fmt.Errorf("unsupport OS: %q", goos)
	}

	err := downloadPackage(downloadUrl)
	if err != nil {
		return "", err
	}
	dest := gogetenv("GOROOT")
	return unzipSource(Name, dest)
}

func Exists() bool {
	_, err := env.LookUpProtoc()
	return err == nil
}