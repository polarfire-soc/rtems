# RTEMS_CHECK_BSPDIR(RTEMS_BSP)
AC_DEFUN([RTEMS_CHECK_BSPDIR],
[
  RTEMS_BSP_ALIAS(ifelse([$1],,[${RTEMS_BSP}],[$1]),bspdir)
  case "$bspdir" in
  dmv177 )
    AC_CONFIG_SUBDIRS([dmv177]);;
  eth_comm )
    AC_CONFIG_SUBDIRS([eth_comm]);;
  helas403 )
    AC_CONFIG_SUBDIRS([helas403]);;
  mbx8xx )
    AC_CONFIG_SUBDIRS([mbx8xx]);;
  motorola_powerpc )
    AC_CONFIG_SUBDIRS([motorola_powerpc]);;
  mpc8260ads )
    AC_CONFIG_SUBDIRS([mpc8260ads]);;
  papyrus )
    AC_CONFIG_SUBDIRS([papyrus]);;
  ppcn_60x )
    AC_CONFIG_SUBDIRS([ppcn_60x]);;
  psim )
    AC_CONFIG_SUBDIRS([psim]);;
  score603e )
    AC_CONFIG_SUBDIRS([score603e]);;
  *)
    AC_MSG_ERROR([Invalid BSP]);;
  esac
])
