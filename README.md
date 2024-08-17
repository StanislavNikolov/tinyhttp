# Usage
Just download the `http.h` file and include it in your source.
```c
#include "http.h"

void home(struct Request *r) {
	// Call once before http_writestr() or http_write().
	http_status(r, 200);

	// Responding with simple strings.
	http_write(r, "<a href='/404'>Missing</a>\n");
	http_write(r, "<a href='/stackbytes'>Whatever is on the stack</a>\n");
}

void stackbytes(struct Request *r) {
	http_status(r, 200);

	// Responding with buffers.
	char buf[128];
	http_writebuf(r, buf, 128);
}

int main() {
	http_route("/", home);
	http_route("/stackbytes", stackbytes);
	http_listen(3333);
	return 0;
}
```


# TODO
 - [ ] Benchmark and optimize if needed
 - [ ] Implement http_get_header()
 - [ ] Implement http_write_json() with "reflection" or whatever the macro hell that iterates over struct fields is called.
 - [ ] Show examples of how to do middleware
 - [ ] Router should be able to support paths with parameters (GET /users/*/friends) and then get parameters with http_get_param(0)
