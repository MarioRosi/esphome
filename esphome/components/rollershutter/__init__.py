import logging
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor

from esphome.const import (CONF_ID, CONF_NAME)
MULTI_CONF = True

AUTO_LOAD = ["ethernet", "time", "uart", "i2c", "pcf8574", "binary_sensor", "text_sensor", "switch", "logger"]

CODEOWNERS = ["@esphome/core"]
rollershutter_ns = cg.esphome_ns.namespace("rollershutter")
RL_Time = rollershutter_ns.class_("RL_Time")
RL_SunDowner = rollershutter_ns.class_("RL_SunDowner")
RL_Group = rollershutter_ns.class_("RL_Group")
RollerShutter = rollershutter_ns.class_("RollerShutter", text_sensor.TextSensor)
RollerShutterComponent = rollershutter_ns.class_("RollerShutterComponent", cg.Component)

CONF_RLS_ROOT = "rollershutter"
CONF_RLS_TIMES = "rstimes"
CONF_RLS_TIMES_SU = "secondsup"
CONF_RLS_TIMES_SD = "secondsdown"
CONF_RLS_TIMES_SGD = "secondsgap"

CONF_RLS_GROUPS = "rsgroups"
CONF_RLS_SD = "sundowner"
CONF_RLS_SD_OFF = "offline"
CONF_RLS_SD_MF = "monthfrom"
CONF_RLS_SD_MT = "monthto"
CONF_RLS_SD_GH = "gaphour"
CONF_RLS_SD_GM = "gapminute"
CONF_RLS_SD_UH = "uphour"
CONF_RLS_SD_UM = "upminute"

CONF_RLS_SHUTTERS = "rshutters"
CONF_RLS_SH_GRP = "rsgroup_id"
CONF_RLS_SH_TIM = "rstime_id"
CONF_RLS_SH_IPU = "inputup_id"
CONF_RLS_SH_IPD = "inputdown_id"
CONF_RLS_SH_OSU = "outputup_id"
CONF_RLS_SH_OSD = "outputdown_id"
CONF_RLS_SH_DIS = "display_id"

CONF_RLS_ALLSH = "rshutterAll"
CONF_RLS_ALLSH_IPU = "allinputup_id"
CONF_RLS_ALLSH_IPD = "allinputdown_id"
CONF_RLS_ALLSH_IPH = "allinputhollyday_id"
CONF_RLS_ALLSH_MAS = "allinput_master"
CONF_RLS_ALLSH_SLV = "allinput_slave"

LOGGER = logging.getLogger(__name__)
LOGGER.info("init.py RollerShutter Start")

CONFIG_RLS_TIME = cv.Schema(
    {
        cv.Required(CONF_ID): cv.declare_id(RL_Time),
        cv.Required(CONF_RLS_TIMES_SU): cv.int_range(0, 100, True, True),
        cv.Required(CONF_RLS_TIMES_SD): cv.int_range(0, 100, True, True),
        cv.Required(CONF_RLS_TIMES_SGD): cv.int_range(0, 100, True, True),
    }
)


CONFIG_RLS_SUNDO = cv.Schema(
    {
        cv.Optional(CONF_RLS_SD_OFF): cv.boolean("true"),
        cv.Optional(CONF_RLS_SD_MF): cv.int_range(1, 12, True, True),
        cv.Optional(CONF_RLS_SD_MT): cv.int_range(1, 12, True, True),
        cv.Optional(CONF_RLS_SD_GH): cv.int_range(0, 23, True, True),
        cv.Optional(CONF_RLS_SD_GM): cv.int_range(0, 59, True, True),
        cv.Optional(CONF_RLS_SD_UH): cv.int_range(0, 23, True, True),
        cv.Optional(CONF_RLS_SD_UM): cv.int_range(0, 59, True, True),
    }
)

CONFIG_RLS_GROUP = cv.Schema(
    {
        cv.Required(CONF_ID): cv.declare_id(RL_Group),
        cv.Required(CONF_NAME): cv.valid_name,
        cv.Required(CONF_RLS_SD): cv.ensure_schema(CONFIG_RLS_SUNDO),
    }
)


CONFIG_RLS_SHUTTER = cv.Schema(
    {
        cv.Required(CONF_ID): cv.declare_id(RollerShutter),
        cv.Required(CONF_NAME): cv.valid_name,
        cv.Required(CONF_RLS_SH_GRP): cv.string,
        cv.Required(CONF_RLS_SH_TIM): cv.string,
        cv.Required(CONF_RLS_SH_IPU): cv.string,
        cv.Required(CONF_RLS_SH_IPD): cv.string,
        cv.Required(CONF_RLS_SH_OSU): cv.string,
        cv.Required(CONF_RLS_SH_OSD): cv.string,
        cv.Required(CONF_RLS_SH_DIS): cv.string,
    }
)

CONFIG_RLS_ALLSHUTTER = cv.Schema(
    {
        cv.Optional(CONF_RLS_ALLSH_IPU): cv.string,
        cv.Optional(CONF_RLS_ALLSH_IPD): cv.string,
        cv.Optional(CONF_RLS_ALLSH_IPH): cv.string,
        cv.Optional(CONF_RLS_ALLSH_MAS): cv.boolean,
        cv.Optional(CONF_RLS_ALLSH_SLV): cv.boolean,
    }
)


CONFIG_SCHEMA = cv.COMPONENT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(RollerShutterComponent),
        cv.Required(CONF_NAME): cv.valid_name,
        cv.Required(CONF_RLS_TIMES): cv.ensure_list(CONFIG_RLS_TIME),
        cv.Required(CONF_RLS_GROUPS): cv.ensure_list(CONFIG_RLS_GROUP),
        cv.Required(CONF_RLS_ALLSH): cv.ensure_schema(CONFIG_RLS_ALLSHUTTER),
        cv.Required(CONF_RLS_SHUTTERS): cv.ensure_list(CONFIG_RLS_SHUTTER),
    }
)

#.__str__()

async def to_code(config):    
    LOGGER.info("init.py RollerShutter to_Code Start")
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.SetIdAndName(str(config[CONF_ID]), config[CONF_NAME]))
    for rlstime in config.get(CONF_RLS_TIMES, []):
        cg.add(var.AddTime(
            str(rlstime[CONF_ID]),
            rlstime[CONF_RLS_TIMES_SU],
            rlstime[CONF_RLS_TIMES_SD],
            rlstime[CONF_RLS_TIMES_SGD]
        ))
    for rlsgroup in config.get(CONF_RLS_GROUPS, []):
        rlssundowner = rlsgroup.get(CONF_RLS_SD)
        if CONF_RLS_SD_OFF in rlssundowner:
            cg.add(var.AddGroup(
                str(rlsgroup[CONF_ID]),
                rlsgroup[CONF_NAME]
            ))
        else:
            cg.add(var.AddGroup(
                str(rlsgroup[CONF_ID]),
                rlsgroup[CONF_NAME],
                rlssundowner[CONF_RLS_SD_MF],
                rlssundowner[CONF_RLS_SD_MT],
                rlssundowner[CONF_RLS_SD_GH],
                rlssundowner[CONF_RLS_SD_GM],
                rlssundowner[CONF_RLS_SD_UH],
                rlssundowner[CONF_RLS_SD_UM],
            ))
    if CONF_RLS_ALLSH in config:
        cg.add(var.SetButtons(
            config[CONF_RLS_ALLSH][CONF_RLS_ALLSH_IPU],
            config[CONF_RLS_ALLSH][CONF_RLS_ALLSH_IPD],
            config[CONF_RLS_ALLSH][CONF_RLS_ALLSH_IPH],
            config[CONF_RLS_ALLSH][CONF_RLS_ALLSH_MAS],
            config[CONF_RLS_ALLSH][CONF_RLS_ALLSH_SLV],
        ))
    for rlshutter in config.get(CONF_RLS_SHUTTERS, []):
        cg.add(var.AddShutter(
            str(rlshutter[CONF_ID]),
            rlshutter[CONF_NAME],
            rlshutter[CONF_RLS_SH_GRP],
            rlshutter[CONF_RLS_SH_TIM],
            rlshutter[CONF_RLS_SH_IPU],
            rlshutter[CONF_RLS_SH_IPD],
            rlshutter[CONF_RLS_SH_OSU],
            rlshutter[CONF_RLS_SH_OSD],
            rlshutter[CONF_RLS_SH_DIS],
        ))    