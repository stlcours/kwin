########### next target ###############

set(kwin_killer_helper_SRCS killer.cpp )


add_executable(kwin_killer_helper ${kwin_killer_helper_SRCS})

target_link_libraries(kwin_killer_helper
    Qt5::Widgets
    Qt5::X11Extras
    KF5::Auth
    KF5::I18n
    KF5::WidgetsAddons
)

install(TARGETS kwin_killer_helper DESTINATION ${LIBEXEC_INSTALL_DIR} )
