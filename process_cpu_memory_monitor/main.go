package main

import (
	"fmt"
	"log"
	"os"
	"time"

	"io/ioutil"

	"github.com/shirou/gopsutil/cpu"
	"github.com/shirou/gopsutil/process"
	"gopkg.in/yaml.v3"
)

func getCpuInfo() (cores int32) {
	cpuInfos, err := cpu.Info()
	if err != nil {
		log.Printf("get cpu info failed, err:%v", err)
		return 1
	}
	cores = 0
	for _, ci := range cpuInfos {
		log.Println(ci)
		cores = cores + ci.Cores
	}
	return cores
}

type Yaml struct {
	ProcessList []string `yaml:"process"`
	Interval    int32    `yaml:"interval"`
}

type watchProcessInfo struct {
	pid  int32
	proc *process.Process
}

var processNameIdMap map[string]watchProcessInfo

func updateProcessNameIdMap() {

	for pname := range processNameIdMap {
		processNameIdMap[pname] = watchProcessInfo{}
	}

	processes, _ := process.Processes()
	for _, p := range processes {
		pname, _ := p.Name()

		_, ok := processNameIdMap[pname]
		if ok {
			processNameIdMap[pname] = watchProcessInfo{p.Pid, p}
		}
	}
}

func loadCfg(conf *Yaml) {
	file, err := ioutil.ReadFile("cfg.yaml")
	if err != nil {
		panic(err)
	}

	err = yaml.Unmarshal(file, conf)
	if err != nil {
		fmt.Println(err)
		return
	}
	log.Printf("conf:%+v", conf)
}

func isFileExist(filepath string) (bool, error) {
	_, err := os.Stat(filepath)
	if err == nil {
		return true, nil
	}
	if os.IsNotExist(err) {
		return false, nil
	}
	return false, err
}

func main() {

	logfilename := "./zpsutil" + time.Now().Format("2006-01-02") + ".log"
	fmt.Println(logfilename)

	// 尝试重命名该文件, 失败则退出, 避免该进程同时多次运行
	isExits, _ := isFileExist(logfilename)
	if isExits {
		err := os.Rename(logfilename, logfilename+"."+time.Now().Format("150411"))
		if err != nil {
			return
		}
	}

	logFile, err := os.OpenFile(logfilename, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0644)
	if err != nil {
		fmt.Println("open log file failed, err:", err)
		return
	}
	log.SetOutput(logFile)
	log.SetFlags(log.Lmicroseconds | log.Ldate)

	cores := getCpuInfo()

	conf := Yaml{}
	loadCfg(&conf)

	processNameIdMap = make(map[string]watchProcessInfo)
	for i := range conf.ProcessList {
		processNameIdMap[conf.ProcessList[i]] = watchProcessInfo{}
	}

	updateProcessNameIdMap()

	for {
		curlogfilename := "./zpsutil" + time.Now().Format("2006-01-02") + ".log"

		if curlogfilename != logfilename {
			fmt.Println(curlogfilename)
			logfilename = curlogfilename
			logFile, err := os.OpenFile(logfilename, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0644)
			if err != nil {
				fmt.Println("open log file failed, err:", err)
				return
			}
			log.SetOutput(logFile)
			log.SetFlags(log.Lmicroseconds | log.Ldate)
		}

		//log.Printf("%+v\n", processNameIdMap)

		var isShouldUpdate = false

		for pname := range processNameIdMap {
			watchProc := processNameIdMap[pname]
			if watchProc.proc != nil {
				runState, _ := watchProc.proc.IsRunning()
				if runState {
					cpu, _ := watchProc.proc.CPUPercent()
					memoryInfoStat, _ := watchProc.proc.MemoryInfo()
					log.Printf("%v cpu percent:%v, memory rss:%v, memory vms:%v\n", pname, cpu/float64(cores), memoryInfoStat.RSS, memoryInfoStat.VMS)
				}
				if !runState {
					log.Printf("%v IsRunning:%v\n", pname, runState)
					isShouldUpdate = true
				}
			} else {
				isShouldUpdate = true
			}
		}

		if isShouldUpdate {
			log.Printf("updateProcessNameIdMap...\n")
			updateProcessNameIdMap()
		}

		var interval time.Duration = time.Duration(int64(1000000000) * int64(conf.Interval))
		time.Sleep(interval)
	}
}
