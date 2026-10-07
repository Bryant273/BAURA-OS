package main

/*
#include <stdint.h>
#include <stdlib.h>

typedef void (*BauraGoBusCallback)(uint32_t topic, const char *data, int size);
static inline void baura_call_bus_callback(void *callback, uint32_t topic,
                                           const char *data, int size) {
    ((BauraGoBusCallback)callback)(topic, data, size);
}
*/
import "C"

import (
	"sync"
	"unsafe"
)

type subscriber struct {
	topic    uint32
	callback unsafe.Pointer
}

var (
	subscribers []subscriber
	mu          sync.RWMutex
)

//export baura_bus_publish
func baura_bus_publish(topic C.uint32_t, data *C.char, size C.int) C.int {
	if topic == 0 || size < 0 || (size > 0 && data == nil) {
		return -1
	}
	var payload []byte
	if size > 0 {
		payload = C.GoBytes(unsafe.Pointer(data), size)
	}
	mu.RLock()
	targets := make([]unsafe.Pointer, 0, len(subscribers))
	for _, sub := range subscribers {
		if sub.topic == uint32(topic) {
			targets = append(targets, sub.callback)
		}
	}
	mu.RUnlock()
	if len(targets) == 0 {
		return 0
	}

	var payloadPointer *C.char
	if len(payload) != 0 {
		payloadPointer = (*C.char)(C.CBytes(payload))
		defer C.free(unsafe.Pointer(payloadPointer))
	}
	for _, callback := range targets {
		C.baura_call_bus_callback(unsafe.Pointer(callback), topic,
			payloadPointer, size)
	}
	return 0
}

//export baura_bus_subscribe
func baura_bus_subscribe(topic C.uint32_t, callback unsafe.Pointer) C.int {
	if topic == 0 || callback == nil {
		return -1
	}
	mu.Lock()
	defer mu.Unlock()
	if len(subscribers) >= 64 {
		return -2
	}
	subscribers = append(subscribers, subscriber{
		topic:    uint32(topic),
		callback: callback,
	})
	return 0
}

//export baura_bus_unsubscribe
func baura_bus_unsubscribe(topic C.uint32_t, callback unsafe.Pointer) C.int {
	if topic == 0 || callback == nil {
		return -1
	}
	mu.Lock()
	defer mu.Unlock()
	for index, sub := range subscribers {
		if sub.topic == uint32(topic) && sub.callback == callback {
			subscribers = append(subscribers[:index], subscribers[index+1:]...)
			return 0
		}
	}
	return -3
}

func main() {}
