# -------------------------------------------------
# Install configuration

# Only list install files that have actually changed.
SET(CMAKE_INSTALL_MESSAGE "LAZY")

# Set confdir and datadir.
IF (NOT DEFINED CONFDIR)
	SET (CONFDIR "${CMAKE_INSTALL_PREFIX}/etc")
ENDIF (NOT DEFINED CONFDIR)

IF (NOT DEFINED DATADIR)
	SET (DATADIR "${CMAKE_INSTALL_PREFIX}/share/opencog")
ENDIF (NOT DEFINED DATADIR)

ADD_DEFINITIONS(-DCONFDIR="${CONFDIR}")
ADD_DEFINITIONS(-DDATADIR="${DATADIR}")

# (re?)define MAN_INSTALL_DIR
SET (MAN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/man")
