import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import core
from esphome.const import (
    CONF_ID,
    CONF_PATH,
    CONF_WIDTH,
    CONF_HEIGHT,
    CONF_RESIZE,
    CONF_TYPE,
)
from esphome.components.display import DisplayRef

CODEOWNERS = ["@yourusername"]
DEPENDENCIES = ["display"]

sd_image_ns = cg.esphome_ns.namespace("sd_image")
SDImage = sd_image_ns.class_("SDImage", cg.Component)

ImageType = sd_image_ns.enum("ImageType")
IMAGE_TYPES = {
    "BINARY": ImageType.IMAGE_TYPE_BINARY,
    "GRAYSCALE": ImageType.IMAGE_TYPE_GRAYSCALE,
    "RGB565": ImageType.IMAGE_TYPE_RGB565,
    "RGB": ImageType.IMAGE_TYPE_RGB,
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(SDImage),
    cv.Required(CONF_PATH): cv.string,
    cv.Optional(CONF_WIDTH): cv.positive_int,
    cv.Optional(CONF_HEIGHT): cv.positive_int,
    cv.Optional(CONF_RESIZE): cv.dimensions,
    cv.Optional(CONF_TYPE, default="RGB565"): cv.enum(IMAGE_TYPES, upper=True),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add(var.set_path(config[CONF_PATH]))
    cg.add(var.set_image_type(config[CONF_TYPE]))
    
    if CONF_RESIZE in config:
        cg.add(var.set_resize(config[CONF_RESIZE][0], config[CONF_RESIZE][1]))
    elif CONF_WIDTH in config and CONF_HEIGHT in config:
        cg.add(var.set_resize(config[CONF_WIDTH], config[CONF_HEIGHT]))
