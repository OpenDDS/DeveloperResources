
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Chat
// ------------------------------
#ifndef CHAT_EXPORT_H
#define CHAT_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (CHAT_HAS_DLL)
#  define CHAT_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && CHAT_HAS_DLL */

#if !defined (CHAT_HAS_DLL)
#  define CHAT_HAS_DLL 1
#endif /* ! CHAT_HAS_DLL */

#if defined (CHAT_HAS_DLL) && (CHAT_HAS_DLL == 1)
#  if defined (CHAT_BUILD_DLL)
#    define Chat_Export ACE_Proper_Export_Flag
#    define CHAT_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define CHAT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* CHAT_BUILD_DLL */
#    define Chat_Export ACE_Proper_Import_Flag
#    define CHAT_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define CHAT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* CHAT_BUILD_DLL */
#else /* CHAT_HAS_DLL == 1 */
#  define Chat_Export
#  define CHAT_SINGLETON_DECLARATION(T)
#  define CHAT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* CHAT_HAS_DLL == 1 */

// Set CHAT_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (CHAT_NTRACE)
#  if (ACE_NTRACE == 1)
#    define CHAT_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define CHAT_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !CHAT_NTRACE */

#if (CHAT_NTRACE == 1)
#  define CHAT_TRACE(X)
#else /* (CHAT_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define CHAT_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (CHAT_NTRACE == 1) */

#endif /* CHAT_EXPORT_H */

// End of auto generated file.
