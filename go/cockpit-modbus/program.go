// program.go
package main

import (
	"encoding/json"
	"os/exec"
)

func runSystemctl(args ...string) ([]byte, error) {
	out, err := exec.Command("sudo", append([]string{"systemctl"}, args...)...).CombinedOutput()
	if err != nil {
		return json.Marshal(map[string]interface{}{"status": false, "error": string(out)})
	}
	return json.Marshal(map[string]bool{"status": true})
}

func isProgramRunning(name string) ([]byte, error) {
	return runSystemctl("is-active", name+".service")
}

func startProgram(name string) ([]byte, error) {
	_, _ = runSystemctl("enable", name+".service")
	return runSystemctl("start", name+".service")
}

func stopProgram(name string) ([]byte, error) {
	_, _ = runSystemctl("disable", name+".service")
	return runSystemctl("stop", name+".service")
}

func restartProgram(name string) ([]byte, error) {
	_, _ = stopProgram(name)
	return startProgram(name)
}
