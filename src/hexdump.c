#include "hexdump.h"

#define BYTES_PER_LINE 16

void hexdump(const char *tag, const void *buffer, uint16_t buff_len) {
    if (buff_len == 0) {
        return;
    }
    // char temp_buffer[BYTES_PER_LINE + 3];  // for not-byte-accessible memory
    const char *ptr_line;
    // format: field[length]
    //  ADDR[10]+"   "+DATA_HEX[8*3]+" "+DATA_HEX[8*3]+"  |"+DATA_CHAR[8]+"|"
    char hd_buffer[10 + 3 + BYTES_PER_LINE * 3 + 3 + BYTES_PER_LINE + 1 + 1];
    char *ptr_hd;
    int bytes_cur_line;

    do {
        if (buff_len > BYTES_PER_LINE) {
            bytes_cur_line = BYTES_PER_LINE;
        } else {
            bytes_cur_line = buff_len;
        }
        // if (!esp_ptr_byte_accessible(buffer)) {
        //     // use memcpy to get around alignment issue
        //     memcpy(temp_buffer, buffer, (bytes_cur_line + 3) / 4 * 4);
        //     ptr_line = temp_buffer;
        // } else {
        ptr_line = buffer;
        // }
        ptr_hd = hd_buffer;

        // ptr_hd += sprintf(ptr_hd, "%p ", buffer);
        for (int i = 0; i < BYTES_PER_LINE; i++) {
            if ((i & 7) == 0) {
                ptr_hd += sprintf(ptr_hd, " ");
            }
            if (i < bytes_cur_line) {
                ptr_hd += sprintf(ptr_hd, " %02x", ptr_line[i]);
            } else {
                ptr_hd += sprintf(ptr_hd, "   ");
            }
        }
        ptr_hd += sprintf(ptr_hd, "  |");
        for (int i = 0; i < bytes_cur_line; i++) {
            if (isprint((int)ptr_line[i])) {
                ptr_hd += sprintf(ptr_hd, "%c", ptr_line[i]);
            } else {
                ptr_hd += sprintf(ptr_hd, ".");
            }
        }
        ptr_hd += sprintf(ptr_hd, "|");

        printf("%s: %s\n", tag, hd_buffer);
        buffer += bytes_cur_line;
        buff_len -= bytes_cur_line;
    } while (buff_len);
}