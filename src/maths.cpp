#include "maths.h"

/// Rect ///////////////////////////////////////////////////////////////////////

Rectangle RectPad(const Rectangle rec, float pad)
{
    Rectangle padded = { rec.x - pad, rec.y - pad, rec.width + pad * 2.0f, rec.height + pad * 2.0f };
    return padded;
}

Rectangle RectPadXY(const Rectangle rec, float padX, float padY)
{
    Rectangle padded = { rec.x - padX, rec.y - padY, rec.width + padX * 2.0f, rec.height + padY * 2.0f };
    return padded;
}