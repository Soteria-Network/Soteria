// Copyright (c) 2018 The Bitcoin Core developers
// Copyright (c) 2025-2026 The Soteria Core developer

#ifndef SOTERIA_UTIL_THREADNAMES_H
#define SOTERIA_UTIL_THREADNAMES_H

#include <string>

namespace util {
//! Rename a thread both in terms of an internal (in-memory) name as well
//! as its system thread name.
//! @note Do not call this for the main thread, as this will interfere with
//! UNIX utilities such as top and killall. Use ThreadSetInternalName instead.
void ThreadRename(std::string&&);

//! Set the internal (in-memory) name of the current thread only.
void ThreadSetInternalName(std::string&&);

//! Get the thread's internal (in-memory) name; used e.g. for identification in
//! logging.
const std::string& ThreadGetInternalName();

} // namespace util

#endif
