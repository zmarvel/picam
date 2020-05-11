package main

import (
	"testing"
	"time"
)

func TestFormatDayDir(t *testing.T) {
	// 1589144404.001234000 = 2020-05-10 15:00:04.001234000
	actual := formatDayDir(time.Unix(1589144404, 1234000))
	expected := "2020-05-10"
	if actual != expected {
		t.Errorf("Expected %s, got %s\n", expected, actual)
	}
}

func TestFormatImageFilename(t *testing.T) {
	// 1589144404.001234000 = 2020-05-10 15:00:04.001234000
	actual := formatImageFilename(time.Unix(1589144404, 1234000))
	expected := "150004.001.png"
	if actual != expected {
		t.Errorf("Expected %s, got %s\n", expected, actual)
	}
}

func TestFormatMetaFilename(t *testing.T) {
	// 1589144404.001234000 = 2020-05-10 15:00:04.001234000
	actual := formatMetaFilename(time.Unix(1589144404, 1234000))
	expected := "150004.001.json"
	if actual != expected {
		t.Errorf("Expected %s, got %s\n", expected, actual)
	}
}
