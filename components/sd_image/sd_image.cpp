#include "sd_image.h"
#include "esphome/core/log.h"
#include <cstring>

#ifdef USE_ESP_IDF
#include <stdio.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#endif

namespace esphome {
namespace sd_image {

static const char *const TAG = "sd_image";

void SDImage::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SD Image...");
  this->load_image();
}

void SDImage::dump_config() {
  ESP_LOGCONFIG(TAG, "SD Image:");
  ESP_LOGCONFIG(TAG, "  Path: %s", this->path_.c_str());
  ESP_LOGCONFIG(TAG, "  Type: %d", this->type_);
  ESP_LOGCONFIG(TAG, "  Transparency: %d", this->transparency_);
  ESP_LOGCONFIG(TAG, "  Dither: %d", this->dither_);
  ESP_LOGCONFIG(TAG, "  Invert Alpha: %s", this->invert_alpha_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Big Endian: %s", this->big_endian_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Size: %dx%d", this->width_, this->height_);
  ESP_LOGCONFIG(TAG, "  Loaded: %s", this->loaded_ ? "YES" : "NO");
}

bool SDImage::load_image() {
  if (this->loaded_) {
    return true;
  }
  
  ESP_LOGI(TAG, "Loading image from: %s", this->path_.c_str());
  
  // Vérifier si le fichier existe
  struct stat st;
  if (stat(this->path_.c_str(), &st) != 0) {
    ESP_LOGE(TAG, "File not found: %s", this->path_.c_str());
    return false;
  }
  
  // Déterminer le format par extension
  std::string ext = this->path_.substr(this->path_.find_last_of('.') + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  
  bool success = false;
  if (ext == "jpg" || ext == "jpeg") {
    success = this->load_jpeg();
  } else if (ext == "png") {
    success = this->load_png();
  } else {
    ESP_LOGE(TAG, "Unsupported image format: %s", ext.c_str());
    return false;
  }
  
  if (success) {
    ESP_LOGI(TAG, "Successfully loaded %dx%d image", this->width_, this->height_);
    this->loaded_ = true;
  }
  
  return success;
}

#ifdef USE_ESP_IDF

bool SDImage::load_jpeg() {
  FILE *file = fopen(this->path_.c_str(), "rb");
  if (!file) {
    ESP_LOGE(TAG, "Failed to open JPEG file: %s", this->path_.c_str());
    return false;
  }
  
  // Obtenir la taille du fichier
  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  // Lire le fichier
  uint8_t *jpeg_buffer = (uint8_t *)malloc(file_size);
  if (!jpeg_buffer) {
    fclose(file);
    ESP_LOGE(TAG, "Failed to allocate JPEG buffer");
    return false;
  }
  
  fread(jpeg_buffer, 1, file_size, file);
  fclose(file);
  
  // Configuration du décodeur
  esp_jpeg_image_cfg_t jpeg_cfg = {
    .indata = jpeg_buffer,
    .indata_size = file_size,
    .outbuf = nullptr,
    .outbuf_size = 0,
    .out_format = JPEG_IMAGE_FORMAT_RGB888,
    .out_scale = JPEG_IMAGE_SCALE_0,
    .flags = {
      .swap_color_bytes = 0,
    }
  };
  
  // Décoder
  esp_jpeg_image_output_t outimg;
  esp_err_t ret = esp_jpeg_decode(&jpeg_cfg, &outimg);
  
  if (ret == ESP_OK) {
    this->process_decoded_data(outimg.outbuf, outimg.width, outimg.height, 3);
    free(outimg.outbuf);
    free(jpeg_buffer);
    return true;
  } else {
    ESP_LOGE(TAG, "JPEG decode failed: %s", esp_err_to_name(ret));
    free(jpeg_buffer);
    return false;
  }
}

bool SDImage::load_png() {
  FILE *file = fopen(this->path_.c_str(), "rb");
  if (!file) {
    ESP_LOGE(TAG, "Failed to open PNG file: %s", this->path_.c_str());
    return false;
  }
  
  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  uint8_t *png_buffer = (uint8_t *)malloc(file_size);
  if (!png_buffer) {
    fclose(file);
    ESP_LOGE(TAG, "Failed to allocate PNG buffer");
    return false;
  }
  
  fread(png_buffer, 1, file_size, file);
  fclose(file);
  
  esp_png_image_cfg_t png_cfg = {
    .indata = png_buffer,
    .indata_size = file_size,
    .outbuf = nullptr,
    .outbuf_size = 0,
    .out_format = PNG_IMAGE_FORMAT_RGB888,
    .flags = {
      .swap_color_bytes = 0,
    }
  };
  
  esp_png_image_output_t outimg;
  esp_err_t ret = esp_png_decode(&png_cfg, &outimg);
  
  if (ret == ESP_OK) {
    this->process_decoded_data(outimg.outbuf, outimg.width, outimg.height, 3);
    free(outimg.outbuf);
    free(png_buffer);
    return true;
  } else {
    ESP_LOGE(TAG, "PNG decode failed: %s", esp_err_to_name(ret));
    free(png_buffer);
    return false;
  }
}

#else

bool SDImage::load_jpeg() {
  ESP_LOGE(TAG, "JPEG decoding requires ESP-IDF framework");
  return false;
}

bool SDImage::load_png() {
  ESP_LOGE(TAG, "PNG decoding requires ESP-IDF framework");
  return false;
}

#endif

void SDImage::process_decoded_data(uint8_t *data, int width, int height, int channels) {
  // Appliquer le redimensionnement si nécessaire
  int target_width = (this->resize_width_ > 0) ? this->resize_width_ : width;
  int target_height = (this->resize_height_ > 0) ? this->resize_height_ : height;
  
  this->width_ = target_width;
  this->height_ = target_height;
  
  // Libérer l'ancienne data si elle existe
  if (this->image_data_) {
    free(this->image_data_);
    this->image_data_ = nullptr;
  }
  
  // Convertir selon le type d'image demandé
  switch (this->type_) {
    case IMAGE_TYPE_BINARY:
      this->convert_to_binary(data, width, height, channels);
      break;
    case IMAGE_TYPE_GRAYSCALE:
      this->convert_to_grayscale(data, width, height, channels);
      break;
    case IMAGE_TYPE_RGB565:
      this->convert_to_rgb565(data, width, height, channels);
      break;
    case IMAGE_TYPE_RGB:
      this->convert_to_rgb(data, width, height, channels);
      break;
  }
}

void SDImage::convert_to_binary(uint8_t *src, int width, int height, int channels) {
  int target_width = this->width_;
  int target_height = this->height_;
  
  // Binary: 1 bit par pixel, aligné sur des bytes
  this->data_size_ = ((target_width + 7) / 8) * target_height;
  this->image_data_ = (uint8_t *)calloc(this->data_size_, 1);
  
  for (int y = 0; y < target_height; y++) {
    for (int x = 0; x < target_width; x++) {
      // Calcul des coordonnées source avec mise à l'échelle
      int src_x = (x * width) / target_width;
      int src_y = (y * height) / target_height;
      int src_idx = (src_y * width + src_x) * channels;
      
      // Conversion en niveau de gris puis binaire
      uint8_t gray = (src[src_idx] + src[src_idx + 1] + src[src_idx + 2]) / 3;
      bool pixel = gray > 128;
      
      if (pixel) {
        int byte_idx = y * ((target_width + 7) / 8) + x / 8;
        int bit_idx = 7 - (x % 8);
        this->image_data_[byte_idx] |= (1 << bit_idx);
      }
    }
  }
}

void SDImage::convert_to_grayscale(uint8_t *src, int width, int height, int channels) {
  int target_width = this->width_;
  int target_height = this->height_;
  
  this->data_size_ = target_width * target_height;
  this->image_data_ = (uint8_t *)malloc(this->data_size_);
  
  for (int y = 0; y < target_height; y++) {
    for (int x = 0; x < target_width; x++) {
      int src_x = (x * width) / target_width;
      int src_y = (y * height) / target_height;
      int src_idx = (src_y * width + src_x) * channels;
      
      uint8_t gray = (src[src_idx] + src[src_idx + 1] + src[src_idx + 2]) / 3;
      this->image_data_[y * target_width + x] = gray;
    }
  }
}

void SDImage::convert_to_rgb565(uint8_t *src, int width, int height, int channels) {
  int target_width = this->width_;
  int target_height = this->height_;
  
  this->data_size_ = target_width * target_height * 2;
  this->image_data_ = (uint8_t *)malloc(this->data_size_);
  uint16_t *rgb565_data = (uint16_t *)this->image_data_;
  
  for (int y = 0; y < target_height; y++) {
    for (int x = 0; x < target_width; x++) {
      int src_x = (x * width) / target_width;
      int src_y = (y * height) / target_height;
      int src_idx = (src_y * width + src_x) * channels;
      
      uint8_t r = src[src_idx] >> 3;      // 5 bits
      uint8_t g = src[src_idx + 1] >> 2;  // 6 bits  
      uint8_t b = src[src_idx + 2] >> 3;  // 5 bits
      
      uint16_t rgb565 = (r << 11) | (g << 5) | b;
      rgb565_data[y * target_width + x] = rgb565;
    }
  }
}

void SDImage::convert_to_rgb(uint8_t *src, int width, int height, int channels) {
  int target_width = this->width_;
  int target_height = this->height_;
  
  this->data_size_ = target_width * target_height * 3;
  this->image_data_ = (uint8_t *)malloc(this->data_size_);
  
  for (int y = 0; y < target_height; y++) {
    for (int x = 0; x < target_width; x++) {
      int src_x = (x * width) / target_width;
      int src_y = (y * height) / target_height;
      int src_idx = (src_y * width + src_x) * channels;
      int dst_idx = (y * target_width + x) * 3;
      
      this->image_data_[dst_idx] = src[src_idx];         // R
      this->image_data_[dst_idx + 1] = src[src_idx + 1]; // G
      this->image_data_[dst_idx + 2] = src[src_idx + 2]; // B
    }
  }
}

void SDImage::draw(display::Display *display, int x, int y) {
  if (!this->load_image()) {
    return;
  }
  
  // Implémentation basique - à adapter selon votre display
  for (int dy = 0; dy < this->height_; dy++) {
    for (int dx = 0; dx < this->width_; dx++) {
      Color color;
      
      switch (this->type_) {
        case IMAGE_TYPE_RGB565: {
          uint16_t *rgb565_data = (uint16_t *)this->image_data_;
          uint16_t pixel = rgb565_data[dy * this->width_ + dx];
          uint8_t r = ((pixel >> 11) & 0x1F) << 3;
          uint8_t g = ((pixel >> 5) & 0x3F) << 2;
          uint8_t b = (pixel & 0x1F) << 3;
          color = Color(r, g, b);
          break;
        }
        case IMAGE_TYPE_RGB: {
          int idx = (dy * this->width_ + dx) * 3;
          color = Color(this->image_data_[idx], 
                       this->image_data_[idx + 1], 
                       this->image_data_[idx + 2]);
          break;
        }
        // ... autres formats
      }
      
      display->draw_pixel_at(x + dx, y + dy, color);
    }
  }
}

void SDImage::draw(display::Display *display, int x, int y, Color color_on, Color color_off) {
  // Version pour images binaires
  this->draw(display, x, y);
}

}  // namespace sd_image
}  // namespace esphome
