import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_NAME

MULTI_CONF = True

AUTO_LOAD = ["ethernet", "time", "uart", "i2c", "pcf8574", "binary_sensor", "switch"]

CODEOWNERS = ["@esphome/core"]
rollershutter_ns = cg.esphome_ns.namespace("rollershutter")
RL_Time = rollershutter_ns.class_("RL_Time")
RL_SunDowner = rollershutter_ns.class_("RL_SunDowner")
RL_Group = rollershutter_ns.class_("RL_Group")
RollerShutter = rollershutter_ns.class_("RollerShutter")
RollerShutterComponent = rollershutter_ns.class_("RollerShutterComponent", cg.Component)

CONF_RLS_ROOT = "rollershutter"
CONF_RLS_TIMES = "rstimes"
CONF_RLS_TIMES_MSU = "milisecondsup"
CONF_RLS_TIMES_MSD = "milisecondsdown"
CONF_RLS_TIMES_MSGD = "milisecondsgap"

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

CONF_RLS_ALLSH = "rshutterAll"
CONF_RLS_ALLSH_IPU = "allinputup_id"
CONF_RLS_ALLSH_IPD = "allinputdown_id"
CONF_RLS_ALLSH_IPH = "allinputhollyday_id"
CONF_RLS_ALLSH_MAS = "allinput_master"
CONF_RLS_ALLSH_SLV = "allinput_slave"


CONFIG_RLS_TIME = cv.Schema(
    {
        cv.Required(CONF_ID): cv.declare_id(RL_Time),
        cv.Required(CONF_RLS_TIMES_MSU): cv.int_range(0, 100000, True, True),
        cv.Required(CONF_RLS_TIMES_MSD): cv.int_range(0, 100000, True, True),
        cv.Required(CONF_RLS_TIMES_MSGD): cv.int_range(0, 100000, True, True),
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
        cv.Required(CONF_RLS_SH_GRP): cv.declare_id(RL_Group),
        cv.Required(CONF_RLS_SH_TIM): cv.declare_id(RL_Time),
        cv.Required(CONF_RLS_SH_IPU): cv.string,
        cv.Required(CONF_RLS_SH_IPD): cv.string,
        cv.Required(CONF_RLS_SH_OSU): cv.string,
        cv.Required(CONF_RLS_SH_OSD): cv.string,
    }
)

CONFIG_RLS_ALLSHUTTER = cv.Schema(
    {
        cv.Optional(CONF_RLS_ALLSH_IPU): cv.string,
        cv.Optional(CONF_RLS_ALLSH_IPD): cv.string,
        cv.Optional(CONF_RLS_ALLSH_IPH): cv.string,
        cv.Optional(CONF_RLS_ALLSH_MAS): cv.boolean(False),
        cv.Optional(CONF_RLS_ALLSH_SLV): cv.boolean(False),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(RollerShutterComponent),
        cv.Required(CONF_RLS_TIMES): cv.ensure_list(CONFIG_RLS_TIME),
        cv.Required(CONF_RLS_GROUPS): cv.ensure_list(CONFIG_RLS_GROUP),
        cv.Required(CONF_RLS_ALLSH): cv.ensure_schema(CONFIG_RLS_ALLSHUTTER),
        cv.Required(CONF_RLS_SHUTTERS): cv.ensure_list(CONFIG_RLS_SHUTTER),
    }
)

# .extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    if CONF_RLS_ROOT in config:
        if rlsRoot := config.get(CONF_RLS_ROOT):
            rlstimes = []
            for rlstime in rlsRoot.get(CONF_RLS_TIMES, []):
                rlstimes[rlstime[CONF_ID]] = RL_Time(
                    rlstime[CONF_ID],
                    rlstime[CONF_RLS_TIMES_MSU],
                    rlstime[CONF_RLS_TIMES_MSD],
                    rlstime[CONF_RLS_TIMES_MSGD],
                )
            rlsgroups = []
            for rlsgroup in rlsRoot.get(CONF_RLS_GROUPS, []):
                if CONF_RLS_SD_OFF in rlsgroup:
                    rlsgroups[rlsgroup[CONF_ID]] = RL_Group(
                        rlsgroup[CONF_ID], rlsgroup[CONF_NAME], RL_SunDowner()
                    )
                else:
                    rlsgroups[rlsgroup[CONF_ID]] = RL_Group(
                        rlsgroup[CONF_ID],
                        rlsgroup[CONF_NAME],
                        RL_SunDowner(
                            rlsgroup[CONF_RLS_SD][CONF_RLS_SD_MF],
                            rlsgroup[CONF_RLS_SD][CONF_RLS_SD_MT],
                            rlsgroup[CONF_RLS_SD][CONF_RLS_SD_GH],
                            rlsgroup[CONF_RLS_SD][CONF_RLS_SD_GM],
                            rlsgroup[CONF_RLS_SD][CONF_RLS_SD_UH],
                            rlsgroup[CONF_RLS_SD][CONF_RLS_SD_UM],
                        ),
                    )
            var = cg.new_Pvariable(config[CONF_ID], config[CONF_NAME])
            if CONF_RLS_ALLSH in rlsRoot:
                var.SetButtons(
                    rlsRoot[CONF_RLS_ALLSH][CONF_RLS_ALLSH_IPU],
                    rlsRoot[CONF_RLS_ALLSH][CONF_RLS_ALLSH_IPD],
                    rlsRoot[CONF_RLS_ALLSH][CONF_RLS_ALLSH_IPH],
                    rlsRoot[CONF_RLS_ALLSH][CONF_RLS_ALLSH_MAS],
                    rlsRoot[CONF_RLS_ALLSH][CONF_RLS_ALLSH_SLV],
                )
            await cg.register_component(var, config)
            rlshutters = []
            for rlshutter in rlsRoot.get(CONF_RLS_SHUTTERS, []):
                rlshutters[rlshutter[CONF_ID]] = RollerShutter(
                    rlshutter[CONF_ID],
                    rlshutter[CONF_NAME],
                    rlsgroups[rlshutter[CONF_RLS_SH_GRP]],
                    rlstimes[rlshutter[CONF_RLS_SH_TIM]],
                    var,
                    rlshutter[CONF_RLS_SH_IPU],
                    rlshutter[CONF_RLS_SH_IPD],
                    rlshutter[CONF_RLS_SH_OSU],
                    rlshutter[CONF_RLS_SH_OSD],
                )
            await var.InitialRun()
            cg.add_define("USE_POWER_SUPPLY")
