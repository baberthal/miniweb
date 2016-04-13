include(JMLColors)
macro(JML_MESSAGE _color _messageText)
  message(STATUS "${${_color}}${_messageText}${ColorReset}")
endmacro(JML_MESSAGE)

macro(MACRO_DYNAMIC_TERM_WIDTH _result_variable)
  set(COLUMNS $ENV{COLUMNS})
  if(NOT COLUMNS)
    if(UNIX)
      execute_process(COMMAND "stty" "size"
        OUTPUT_VARIABLE STTY_COLS
        ERROR_FILE /dev/null
        OUTPUT_STRIP_TRAILING_WHITESPACE)
      if(STTY_COLS)
        string(REPLACE " " ";" STTY_COLUMNS ${STTY_COLS})
        list(GET STTY_COLUMNS 1 DYN_COLS)
        set(COLUMNS ${DYN_COLS})
      else()
        execute_process(COMMAND "tput" "cols"
          OUTPUT_VARIABLE TPUT_COLS
          ERROR_FILE /dev/null
          OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(TPUT_COLS)
          set(COLUMNS ${TPUT_COLS})
        endif(TPUT_COLS)
      endif(STTY_COLS)
    else()
      set(COLUMNS 80)
    endif(UNIX)
  endif(NOT COLUMNS)
  set(${_result_variable} ${COLUMNS})
endmacro(MACRO_DYNAMIC_TERM_WIDTH)

function(JML_SEPARATOR)
  if(ARGC)
    if(ARGV0 MATCHES "(Red|Green|Yellow|Blue|Magenta|Cyan|White|Bold)")
      set(COLOR ${ARGV0})
    else()
      set(SEPARATOR ${ARGV0})
    endif(ARGV0 MATCHES "(Red|Green|Yellow|Blue|Magenta|Cyan|White|Bold)")

    if(ARGV1)
      set(SEPARATOR ${ARGV1})
    endif(ARGV1)
  endif(ARGC)

  if(NOT COLOR)
    set(COLOR BoldWhite)
  endif(NOT COLOR)

  if(NOT SEPARATOR)
    set(SEPARATOR "=")
  endif(NOT SEPARATOR)

  macro_dynamic_term_width(COLUMNS)
  string(RANDOM LENGTH ${COLUMNS} ALPHABET ${SEPARATOR} SEPARATOR_MESSAGE)
  message("${${COLOR}}${SEPARATOR_MESSAGE}${ColorReset}")
endfunction(JML_SEPARATOR)

macro(JML_CONFIG_SUMMARY_MESSAGE _variable)
  message(STATUS "${Blue}${_variable}${ColorReset} -- ${White}${${_variable}}${ColorReset}")
endmacro(JML_CONFIG_SUMMARY_MESSAGE)

macro(JML_CONFIG_SUMMARY_MESSAGE_LIST _list)
  set(VARNAME "${Blue}${_list}${ColorReset} -- ")
  string(LENGTH "${_list} --    " OFFSET)

  string(RANDOM LENGTH ${OFFSET} ALPHABET " " OFFSET_SPACES)

  set(LIST_DUP "${${_list}}")
  list(GET LIST_DUP 1 FIRST_ITEM_FULL_PATH)
  get_filename_component(FIRST_ITEM ${FIRST_ITEM_FULL_PATH} NAME)
  list(REMOVE_ITEM LIST_DUP ${FIRST_ITEM_FULL_PATH})

  set(VARMSG "${White}${FIRST_ITEM}${ColorReset}\n")
  foreach(list_var ${LIST_DUP})
    get_filename_component(lv_name ${list_var} NAME)
    string(APPEND VARMSG "${OFFSET_SPACES}${White}${lv_name}${ColorReset}\n")
  endforeach(list_var)

  message(STATUS "${VARNAME}${VARMSG}")

endmacro(JML_CONFIG_SUMMARY_MESSAGE_LIST)

#  vim: set ts=8 sw=2 tw=0 ft=cmake et :
