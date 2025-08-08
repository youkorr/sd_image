#pragma once

#include "esphome/core/component.h"
#include "esphome/core/color.h"
#include "esphome/components/display/display_buffer.h"
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

class SDImage : public Component {
 public:
  void setup() override;
  void dump_config() override;
  
  void set_path(const std::string &path) { this->path_ = path; }
  void set_image_type(ImageType type) { this->type_ = type; }
  void set_resize(int width, int height) { 
    this->resize_width_ = width; 
    this->resize_height_ = height; 
  }
  
  // Méthodes pour dessiner l'image
  void draw(display::Display *display, int x, int y);
  void draw(display::Display *display, int x, int y, Color color_on, Color color_off = Color::BLACK);
  
  // Obtenir les dimensions de l'image
  int get_width() const { return this->width_; }
  int get_height() const { return this->height_; }
  
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
