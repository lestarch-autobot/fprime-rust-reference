#####
# ReferenceDeploymentCdhCoreConfig: overrides the CdhCore subtopology configuration (command dispatcher queue depth).
#####
register_fprime_config(
    "ReferenceDeploymentCdhCoreConfig"
    CONFIGURATION_OVERRIDES
        "${CMAKE_CURRENT_LIST_DIR}/CdhCoreConfig.fpp"
    EXCLUDE_FROM_ALL
    INTERFACE
)
