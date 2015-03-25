package main

import (
	"bytes"
	"crypto/aes"
	"crypto/cipher"
	"encoding/binary"
	"flag"
	"fmt"
	"log"
	"net"
	"net/http"
	"net/http/httputil"
	"os"
	"os/signal"
	"runtime"
	"runtime/pprof"
	"sync"
)

type Cache struct {
	lock sync.RWMutex
	data map[string][]byte
}

func (c *Cache) Get(key string) ([]byte, bool) {
	//	c.lock.RLock()
	//	defer c.lock.RUnlock()
	d, ok := c.data[key]
	return d, ok
}

func (c *Cache) Put(key string, d []byte) {
	//	c.lock.Lock()
	//	defer c.lock.Unlock()
	c.data[key] = d
}

func startProxy() {
	cache := new(Cache)
	cache.data = make(map[string][]byte)

	addr, err := net.ResolveTCPAddr("tcp", ":3128")
	checkError(err)
	listener, err := net.ListenTCP("tcp", addr)
	checkError(err)
	fmt.Println("Listening to [tcp] :3128!")

	// udp_addr, err := net.ResolveUDPAddr("udp", ":3128")
	// checkError(err)
	// udp_conn, err := net.ListenUDP("udp", udp_addr)
	// checkError(err)
	// fmt.Println("Listening to [udp] :3128!")

	for {
		conn, err := listener.AcceptTCP()
		checkError(err)
		go handleClient(conn, cache)
	}
}

func handleClient(conn *net.TCPConn, cache *Cache) {
	// fmt.Println("incoming tcp connection")
	err := conn.SetNoDelay(false)
	checkError(err)
	defer conn.Close()
	for {
		//fmt.Println("handleClient to Read.")
		buf := make([]byte, 8)
		_, err := conn.Read(buf)
		if err != nil {
			fmt.Println("handleClient connection closed.")
			return
		}
		//fmt.Println("handleClient Read.")

		var length, total_len int32
		length_buf := buf[:4]
		total_len_buf := buf[4:]
		binary.Read(bytes.NewBuffer(length_buf), binary.LittleEndian, &length)
		binary.Read(bytes.NewBuffer(total_len_buf), binary.LittleEndian, &total_len)
		//fmt.Println(length)
		//fmt.Println(total_len)

		crypt_buf := make([]byte, length)
		_, err = conn.Read(crypt_buf)

		key := make([]byte, 32)
		iv := make([]byte, 16)
		block, err := aes.NewCipher(key)
		decrypter := cipher.NewCFBDecrypter(block, iv)
		decrypted := make([]byte, length)
		decrypter.XORKeyStream(decrypted, crypt_buf)
		url := string(decrypted)

		//fmt.Println("handleClient cipher Read.")
		// fmt.Println(url)

		buf = make([]byte, total_len)
		_, err = conn.Read(buf)
		if err != nil {
			conn.Close()
			return
		}
		//fmt.Println("handleClient garbage Read.")

		data, found := cache.Get(url)
		if !found {
			// fmt.Println("handleClient cache miss")
			tr := &http.Transport{
				DisableCompression: true,
			}
			client := &http.Client{Transport: tr}
			request, err := http.NewRequest("GET", "http://"+url, nil)
			checkError(err)
			request.Header.Add("Accept-Encoding", "gzip")
			response, err := client.Do(request)
			checkError(err)
			dump, err := httputil.DumpResponse(response, true)
			checkError(err)
			//fmt.Println(string(dump))
			data = dump
			cache.Put(url, data)
		} else {
			// fmt.Println("handleClient cache hit")
		}

		conn.Write(data)
		// fmt.Println("handleClient HTTP resp Write.")

		break
	}
}

func checkError(err error) int {
	if err != nil {
		if err.Error() == "EOF" {
			fmt.Println("Connection Closed.")
			return 0
		}
		log.Fatal("Unknown Error!", err.Error())
		return -1
	}
	return 1
}

var cpuprofile = flag.String("cpuprofile", "", "write cpu profile to file")

func main() {
	runtime.GOMAXPROCS(128)
	flag.Parse()
	if *cpuprofile != "" {
		f, err := os.Create(*cpuprofile)
		if err != nil {
			log.Fatal(err)
		}
		pprof.StartCPUProfile(f)
		defer pprof.StopCPUProfile()

		// Catch CTRL+C and stop the profiling
		c := make(chan os.Signal, 1)
		signal.Notify(c, os.Interrupt)
		go func() {
			for sig := range c {
				log.Printf("captured %v, stopping profiler and exiting..", sig)
				pprof.StopCPUProfile()
				os.Exit(1)
			}
		}()
	}
	//runtime.GOMAXPROCS(16)
	startProxy()
}
