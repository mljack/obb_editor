#if defined(__EMSCRIPTEN__)

#include <emscripten.h>
#include "fs_helper.h"

EM_ASYNC_JS(unsigned char *, do_load2, (const char* file_path, int* num_bytes), {
	const path_js = UTF8ToString(file_path);
	if (!(path_js in app.files)) {
		setValue(num_bytes, 0, 'i32');
		return Module._malloc(1);
	}

  const blob = await app.load_file(path_js);
	let data = new Uint8Array(await blob.arrayBuffer());
	const size = data.length * data.BYTES_PER_ELEMENT;
	var buf = Module._malloc(size+1);
	Module.HEAPU8.set(data, buf);
	Module.HEAPU8.set([0], buf+size);
	setValue(num_bytes, size, 'i32');
	return buf;
});

EM_ASYNC_JS(void, do_save2, (const char* file_path, const char* content), {
	await app.save_file(UTF8ToString(file_path), UTF8ToString(content))
});

unsigned char* do_load(const char* file_path, int* num_bytes) {
  return do_load2(file_path, num_bytes);
}

void do_save(const char* file_path, const char* content) {
  do_save2(file_path, content);
}

#endif