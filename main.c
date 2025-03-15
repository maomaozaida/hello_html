#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

// 解码 QUOTED-PRINTABLE 编码的字符串
void decode_quoted_printable(const char *input, char *output) {
    const char *src = input;
    char *dst = output;
    while (*src) {
        if (*src == '=' && *(src + 1) && *(src + 2)) {
            // 如果是 `=` 开头的十六进制编码
            char hex[3] = { *(src + 1), *(src + 2), '\0' };
            if (isxdigit(hex[0]) && isxdigit(hex[1])) {
                *dst++ = (char)strtol(hex, NULL, 16);
                src += 3;
            } else {
                // 如果不是有效的十六进制字符，直接跳过
                *dst++ = *src++;
            }
        } else if (*src == '=' && (*(src + 1) == '\r' || *(src + 1) == '\n')) {
            // 跳过软换行符（以 `=` 结尾的行）
            src += 2;
        } else {
            // 普通字符直接复制
            *dst++ = *src++;
        }
    }

    // 去除尾部多余的 `=`
    if (dst > output && *(dst - 1) == '=') {
        dst--;
    }

    *dst = '\0'; // 添加字符串结束符
}

// 去除地址中的多余分号
void clean_address(const char *input, char *output) {
    const char *src = input;
    char *dst = output;
    int last_was_semicolon = 0;

    while (*src) {
        if (*src == ';') {
            if (!last_was_semicolon) {
                *dst++ = ' '; // 将分号替换为空格
                last_was_semicolon = 1;
            }
        } else {
            *dst++ = *src;
            last_was_semicolon = 0;
        }
        src++;
    }

    *dst = '\0'; // 添加字符串结束符
}

// 解析并打印VCF文件内容
void parse_vcf(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("无法打开文件");
        return;
    }

    // 生成输出文件名
    char output_name[1024];
    sprintf(output_name, "output_%d.txt", rand());

    // 使用 "wb" 模式以 UTF-8 编码写入文件
    FILE *file2 = fopen(output_name, "wb");
    if (!file2) {
        perror("无法创建输出文件");
        fclose(file);
        return;
    }

    // 写入 UTF-8 BOM（字节顺序标记）
    unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    fwrite(bom, sizeof(bom), 1, file2);

    int i = 1;
    char line[1024];
    char decoded[1024];
    char cleaned[1024];
    printf("解析VCF文件内容:\n");

    while (fgets(line, sizeof(line), file)) {
        // 去除行尾的换行符
        line[strcspn(line, "\r\n")] = '\0';

        if (strstr(line, "FN;CHARSET=UTF-8;ENCODING=QUOTED-PRINTABLE:") == line) {
            decode_quoted_printable(line + strlen("FN;CHARSET=UTF-8;ENCODING=QUOTED-PRINTABLE:"), decoded);
            printf("姓名: %s\n", decoded);
            if (i) {
                fprintf(file2, "姓名: %s\n", decoded);
                i = 0;
            } else {
                fprintf(file2, "\n姓名: %s\n", decoded);
            }
            
        } else if (strstr(line, "TEL;CELL:") == line) {
            printf("电话: %s\n", line + strlen("TEL;CELL:"));
            fprintf(file2, "电话: %s\n", line + strlen("TEL;CELL:"));
        } else if (strstr(line, "EMAIL;HOME:") == line) {
            fprintf(file2, "电子邮件: %s\n", line + strlen("EMAIL;HOME:"));
        } else if (strstr(line, "ADR;HOME;CHARSET=UTF-8;ENCODING=QUOTED-PRINTABLE:") == line) {
            decode_quoted_printable(line + strlen("ADR;HOME;CHARSET=UTF-8;ENCODING=QUOTED-PRINTABLE:"), decoded);
            clean_address(decoded, cleaned); // 清理地址中的多余分号
            printf("地址: %s\n", cleaned);
            fprintf(file2, "地址: %s\n", cleaned);
        }
    }

    fclose(file);
    fclose(file2);
    printf("解析结果已保存到文件: %s\n", output_name);
}

int main(int argc, char *argv[]) {
    SetConsoleOutputCP(65001);

    // 初始化随机数种子
    srand(time(NULL));

    // 检查是否提供了文件路径参数
    if (argc < 2) {
        fprintf(stderr, "用法: %s <文件路径>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1]; // 从命令行参数获取文件路径
    parse_vcf(filename);
    return 0;
}