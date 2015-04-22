package main

import (
	"log"

	"golang.org/x/mobile/app"
	"mojo/public/go/system"
)

//#include "mojo/public/c/system/types.h"
import "C"

//export MojoMain
func MojoMain(handle C.MojoHandle) C.MojoResult {
	log.Printf("GetTimeTicksNow:%v", system.GetCore().GetTimeTicksNow())
	return C.MOJO_RESULT_OK
}

func main() {
	app.Run(app.Callbacks{})
}
