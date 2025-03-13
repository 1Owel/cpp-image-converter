#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <vector>

namespace img_lib {

    using namespace std;

PACKED_STRUCT_BEGIN BitmapFileHeader {
    char signature[2] = {'B', 'M'};
    uint32_t file_size = 0;
    uint32_t reserved = 0;
    uint32_t data_offset = 54;
} PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t header_size = 40;
    int32_t width = 0;
    int32_t height = 0;
    uint16_t planes = 1;
    uint16_t bpp = 24;
    uint32_t compression = 0;
    uint32_t data_size = 0;
    int32_t h_res = 11811;
    int32_t v_res = 11811;
    uint32_t colors = 0;
    uint32_t important_colors = 0x1000000;
} PACKED_STRUCT_END

static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

bool SaveBMP(const Path& file, const Image& image) {
    ofstream out(file, ios::binary);
    if (!out) return false;

    const int w = image.GetWidth();
    const int h = image.GetHeight();
    const int stride = GetBMPStride(w);

    BitmapFileHeader file_header;
    file_header.file_size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + stride * h;

    BitmapInfoHeader info_header;
    info_header.width = w;
    info_header.height = h;
    info_header.data_size = stride * h;

    out.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
    out.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));

    vector<char> buffer(stride);
    for (int y = h - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < w; ++x) {
            buffer[x * 3 + 0] = static_cast<char>(line[x].b);
            buffer[x * 3 + 1] = static_cast<char>(line[x].g);
            buffer[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        out.write(buffer.data(), stride);
    }

    return out.good();
}

Image LoadBMP(const Path& file) {
    ifstream in(file, ios::binary);
    if (!in) return {};

    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;

    // Чтение заголовков
    in.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    in.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    // Проверка сигнатуры и параметров
    if (file_header.signature[0] != 'B' || 
        file_header.signature[1] != 'M' || 
        info_header.bpp != 24 || 
        info_header.compression != 0 ||
        info_header.width <= 0 || 
        info_header.height == 0) {
        return {};
    }

    const int w = info_header.width;
    const int h = abs(info_header.height); // Отрицательная высота
    const int stride = GetBMPStride(w);

    // Создаём изображение
    Image result(w, h, Color::Black());
    vector<char> buffer(stride);

    // Переходим к данным изображения
    in.seekg(file_header.data_offset, ios::beg);

    // Читаем строки снизу вверх
    for (int y = h - 1; y >= 0; --y) {
        if (!in.read(buffer.data(), stride)) {
            return {}; // Ошибка чтения
        }

        Color* line = result.GetLine(y);
        for (int x = 0; x < w; ++x) {
            line[x].b = static_cast<byte>(buffer[x * 3 + 0]);
            line[x].g = static_cast<byte>(buffer[x * 3 + 1]);
            line[x].r = static_cast<byte>(buffer[x * 3 + 2]);
        }
    }

    return result;
}

} // namespace img_lib