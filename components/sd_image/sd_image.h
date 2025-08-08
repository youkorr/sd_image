#pragma once

#include "esphome/core/component.h"
#include "esphome/core/color.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/components/image/image.h"
#include <string>

#ifdef USE_ESP_IDF
#include "esp_jpeg_dec.h"
#include "esp_png_dec.h"
#endif

namespace esphome {
namespace sd_image {

enum ImageType {
  IMAGE_TYPE_BINARY = 0,
  IMAGE_TYPE_GRAYSCALE,
  IMAGE_TYPE_RGB565,
  IMAGE_TYPE_RGB,
};

enum TransparencyType {
  TRANSPARENCY_OPAQUE = 0,
  TRANSPARENCY_CHROMA_KEY,
  TRANSPARENCY_ALPHA_CHANNEL,
};

enum DitherType {
  DITHER_NONE = 0,
  DITHER_FLOYDSTEINBERG,
};

class SDImage : public image::Image {
 public:
  void setup() override;
  void dump_config() override;
  
  void set_path(const std::string &path) { this->path_ = path; }
  void set_image_type(ImageType type) { this->type_ = type; }
  void set_transparency(TransparencyType transparency) { this->transparency_ = transparency; }
  void set_dither(DitherType dither) { this->dither_ = dither; }
  void set_invert_alpha(bool invert) { this->invert_alpha_ = invert; }
  void set_big_endian(bool big_endian) { this->big_endian_ = big_endian; }
  void set_resize(int width, int height) { 
    this->resize_width_ = width; 
    this->resize_height_ = height; 
  }
  
  // Surcharger les méthodes de image::Image
  void draw(int x, int y, display::Display *display, Color color_on, Color color_off) override;
  int get_width() override;
  int get_height() override;
  
  // Forcer le rechargement
  void reload() { this->loaded_ = false; }
  
 protected:
  bool load_image();
  bool load_jpeg();
  bool load_png();
  void process_decoded_data(uint8_t *data, int width, int height, int channels);
  void convert_to_binary(uint8_t *src, int width, int height, int channels);
  void convert_to_grayscale(uint8_t *src, int width, int height, int channels);
  void convert_to_rgb565(uint8_t *src, int width, int height, int channels);
  void convert_to_rgb(uint8_t *src, int width, int height, int channels);
  
  std::string path_;
  ImageType type_ = IMAGE_TYPE_RGB565;
  TransparencyType transparency_ = TRANSPARENCY_OPAQUE;
  DitherType dither_ = DITHER_NONE;
  bool invert_alpha_ = false;
  bool big_endian_ = false;
  int resize_width_ = 0;
  int resize_height_ = 0;
  
  bool loaded_ = false;
  int width_ = 0;
  int height_ = 0;
  uint8_t *image_data_ = nullptr;
  size_t data_size_ = 0;
};

}  // namespace sd_image
}  // namespace esphome
