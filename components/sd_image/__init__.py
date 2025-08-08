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

CONF_TRANSPARENCY = "transparency"
CONF_BYTE_ORDER = "byte_order"
CONF_INVERT_ALPHA = "invert_alpha"
CONF_DITHER = "dither"

TRANSPARENCY_TYPES = {
    "OPAQUE": 0,
    "CHROMA_KEY": 1,
    "ALPHA_CHANNEL": 2,
}

BYTE_ORDER_TYPES = {
    "BIG_ENDIAN": True,
    "LITTLE_ENDIAN": False,
}

DITHER_TYPES = {
    "NONE": 0,
    "FLOYDSTEINBERG": 1,
}

# Schéma pour une seule image
SINGLE_IMAGE_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(SDImage),
    cv.Required(CONF_PATH): cv.string,
    cv.Optional(CONF_WIDTH): cv.positive_int,
    cv.Optional(CONF_HEIGHT): cv.positive_int,
    cv.Optional(CONF_RESIZE): cv.dimensions,
    cv.Optional(CONF_TYPE, default="RGB565"): cv.enum(IMAGE_TYPES, upper=True),
    cv.Optional(CONF_TRANSPARENCY, default="OPAQUE"): cv.enum(TRANSPARENCY_TYPES, upper=True),
    cv.Optional(CONF_BYTE_ORDER, default="LITTLE_ENDIAN"): cv.enum(BYTE_ORDER_TYPES, upper=True),
    cv.Optional(CONF_INVERT_ALPHA, default=False): cv.boolean,
    cv.Optional(CONF_DITHER, default="NONE"): cv.enum(DITHER_TYPES, upper=True),
}).extend(cv.COMPONENT_SCHEMA)

# Schéma principal - peut être une liste d'images
CONFIG_SCHEMA = cv.Any(
    cv.ensure_list(SINGLE_IMAGE_SCHEMA),  # Liste d'images
    SINGLE_IMAGE_SCHEMA,  # Image unique
)

async def to_code(config):
    # Si c'est une liste d'images
    if isinstance(config, list):
        for image_config in config:
            await process_single_image(image_config)
    else:
        await process_single_image(config)

async def process_single_image(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add(var.set_path(config[CONF_PATH]))
    cg.add(var.set_image_type(config[CONF_TYPE]))
    cg.add(var.set_transparency(config[CONF_TRANSPARENCY]))
    cg.add(var.set_dither(config[CONF_DITHER]))
    cg.add(var.set_invert_alpha(config[CONF_INVERT_ALPHA]))
    cg.add(var.set_big_endian(config[CONF_BYTE_ORDER]))
    
    if CONF_RESIZE in config:
        cg.add(var.set_resize(config[CONF_RESIZE][0], config[CONF_RESIZE][1]))
    elif CONF_WIDTH in config and CONF_HEIGHT in config:
        cg.add(var.set_resize(config[CONF_WIDTH], config[CONF_HEIGHT]))
