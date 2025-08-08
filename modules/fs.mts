// fs.mts
export function readFile(path /* takes user input and passes it to __pfm_fs_read() */) {
    return __pfm_fs_read(path);  // Provided by the pfmjs compiler
}

export function writeFile(path, data /* takes user input and passes it to __pfm_fs_write() */) {
    return __pfm_fs_write(path, data);  // Provided by the pfmjs compiler
}