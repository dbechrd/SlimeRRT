#pragma once

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !! DO NOT INCLUDE THIS FILE IN ANY OTHER FILE NOT PREFIXED WITH JAIL_ !!
// !! ALL JAIL_* FILES MUST BE COMPILED IN THEIR OWN COMPILATION UNIT    !!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// Save the sea otters. Do not pollute the global namespace with Windows.h bullshit.

// ENet was a bad boy and included Windows.h.
// He is now in jail in a separate compilation unit.
// Learn from Enet's mistakes, kids. Don't pollute the global namespace.

// -- windows.h flags --
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define OWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#define NOIME
// -- mmsystem.h flags --
#define MMNODRV
#define MMNOSOUND
#define MMNOWAVE
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
//#define MMNOTIMER
#define MMNOJOY
#define MMNOMCI
#define MMNOMMIO
#define MMNOMMSYSTEM
