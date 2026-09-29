
package main

import "C"
import (
	"fmt"
	"sync"
	"unsafe"
)

type Message struct {
	Topic uint32
	Data  []byte
}

type Subscriber func(msg Message)

var (
	subscribers = make(map[uint32][]Subscriber)
	mu          sync.RWMutex
)

//export baura_bus_publish
func baura_bus_publish(topic uint32, data *C.char, size C.int) {
	mu.RLock()
	subs, exists := subscribers[topic]
	mu.RUnlock()

	if !exists {
		return
	}

	// Convert C data to Go slice
	byteData := C.GoBytes(unsafe.Pointer(data), size)
	msg := Message{Topic: topic, Data: byteData}

	// Dispatch to all subscribers asynchronously via goroutines
	for _, sub := range subs {
		go sub(msg)
	}
}

//export baura_bus_subscribe
func baura_bus_subscribe(topic uint32, callback unsafe.Pointer) {
	mu.Lock()
	defer mu.Unlock()

	// In a real OS, we would convert the unsafe.Pointer to a Go function
	// For this implementation, we'll simulate the callback registration
	fmt.Printf("Subscriber registered for topic %d
", topic)

	// Mock callback for demonstration in the core
	dummySub := func(msg Message) {
		fmt.Printf("Topic %d received data: %s\n", msg.Topic, string(msg.Data))
	}

	subscribers[topic] = append(subscribers[topic], dummySub)
}

func main() {
    // This keeps the bus alive as a service
    select {}
}
