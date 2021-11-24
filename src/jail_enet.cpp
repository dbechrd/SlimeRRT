// ENet was a bad boy and included Windows.h.
// He is now in jail a separate compilation unit.
// Learn from Enet's mistakes, kids. Don't pollute the global namespace.
#include "jail_windows_h.h"

// -- winsock2.h flags --
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning(push)
#pragma warning(disable:4100)
#pragma warning(disable:4245)
#pragma warning(disable:4701)
#pragma warning(disable:4996)
#pragma warning(disable:6385)
#pragma warning(disable:26812)
#define ENET_IMPLEMENTATION
#include "enet_zpl.h"
#pragma warning(pop)
#undef far
#undef near
