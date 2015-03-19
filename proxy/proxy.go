package main

import (
	"bytes"
	"crypto/aes"
	"crypto/cipher"
	"encoding/binary"
	"fmt"
	"log"
	"net"
	"net/http"
	"net/http/httputil"
)

type callbackChannel struct {
	insert   bool
	url      string
	data     []byte
	callback chan []byte
}

func cacheManager(msg chan callbackChannel) {
	cache := make(map[string][]byte)
	for {
		m := <-msg
		if m.insert == true {
			cache[m.url] = m.data
		} else {
			m.callback <- cache[m.url]
		}
	}
}

func startProxy() {
	msg_chan := make(chan callbackChannel)
	go cacheManager(msg_chan)

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
		conn, err := listener.Accept()
		checkError(err)
		go handleClient(conn, msg_chan)
	}
}

func handleClient(conn net.Conn, msg_chan chan callbackChannel) {
	fmt.Println("incoming tcp connection")
	defer conn.Close()
	// closed := make(chan bool)
	// go detectClosed(conn, closed)
	for {
		fmt.Println("handleClient to Read.")
		buf := make([]byte, 8)
		_, err := conn.Read(buf)
		if err != nil {
			fmt.Println("handleClient connection closed.")
			return
		}
		fmt.Println("handleClient Read.")

		var length, total_len int32
		length_buf := buf[:4]
		total_len_buf := buf[4:]
		binary.Read(bytes.NewBuffer(length_buf), binary.LittleEndian, &length)
		binary.Read(bytes.NewBuffer(total_len_buf), binary.LittleEndian, &total_len)
		fmt.Println(length)

		crypt_buf := make([]byte, length)
		_, err = conn.Read(crypt_buf)

		key := make([]byte, 32)
		iv := make([]byte, 16)
		block, err := aes.NewCipher(key)
		decrypter := cipher.NewCFBDecrypter(block, iv)
		decrypted := make([]byte, length)
		decrypter.XORKeyStream(decrypted, crypt_buf)
		url := string(decrypted)

		fmt.Println("handleClient cipher Read.")
		fmt.Println(url)

		buf = make([]byte, total_len)
		_, err = conn.Read(buf)
		if err != nil {
			conn.Close()
			return
		}
		fmt.Println("handleClient garbage Read.")

		callback_chan := make(chan []byte)
		m := callbackChannel{insert: false, url: url, data: nil, callback: callback_chan}
		msg_chan <- m
		data := <-callback_chan

		if data == nil {
			fmt.Println("handleClient cache miss")
			resp, err := http.Get("http://" + string(decrypted))
			checkError(err)
			dump, err := httputil.DumpResponse(resp, true)
			checkError(err)
			m = callbackChannel{insert: true, url: url, data: dump, callback: nil}
			msg_chan <- m
			data = dump
		}

		conn.Write(data)
		fmt.Println("handleClient HTTP resp Write.")
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

func main() {
	startProxy()
}
