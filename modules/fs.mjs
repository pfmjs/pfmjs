// fs.js
export function readFile(path) {
    return __pfm_fs_read(path);  // Provided by C
}

export function writeFile(path, data) {
    return __pfm_fs_write(path, data);  // Provided by C
}