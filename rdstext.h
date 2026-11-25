/*
 * Text conversion for RDS (part of FmStick).
 *
 * RDS receivers do not use UTF-8 or Latin-1: PS and RadioText are sent in
 * the RDS character set (EN 50067 / IEC 62106, annex E), one byte per
 * character. Sending UTF-8 shows two wrong characters for every accented
 * letter and can cut a letter in half at the 8 or 64 byte limit.
 *
 * Copyright (C) 2026 Aurel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef RDSTEXT_H
#define RDSTEXT_H

#include <QByteArray>
#include <QHash>
#include <QString>

/* One byte per character, never 0x00, so the result can be passed as a
   C string to the fmtx functions. */
inline QByteArray toRdsCharset(const QString &text)
{
    static const QHash<char32_t, char> table = []() {
        /* bytes 0x80..0xFE of the RDS character set, in order */
        static const char32_t high[] = {
            U'á', U'à', U'é', U'è', U'í', U'ì', U'ó', U'ò', U'ú', U'ù', U'Ñ', U'Ç', U'Ş', U'ß', U'¡', U'Ĳ',
            U'â', U'ä', U'ê', U'ë', U'î', U'ï', U'ô', U'ö', U'û', U'ü', U'ñ', U'ç', U'ş', U'ğ', U'ı', U'ĳ',
            U'ª', U'α', U'©', U'‰', U'Ğ', U'ě', U'ň', U'ő', U'π', U'€', U'£', U'$', U'←', U'↑', U'→', U'↓',
            U'º', U'¹', U'²', U'³', U'±', U'İ', U'ń', U'ű', U'µ', U'¿', U'÷', U'°', U'¼', U'½', U'¾', U'§',
            U'Á', U'À', U'É', U'È', U'Í', U'Ì', U'Ó', U'Ò', U'Ú', U'Ù', U'Ř', U'Č', U'Š', U'Ž', U'Đ', U'Ŀ',
            U'Â', U'Ä', U'Ê', U'Ë', U'Î', U'Ï', U'Ô', U'Ö', U'Û', U'Ü', U'ř', U'č', U'š', U'ž', U'đ', U'ŀ',
            U'Ã', U'Å', U'Æ', U'Œ', U'ŷ', U'Ý', U'Õ', U'Ø', U'Þ', U'Ŋ', U'Ŕ', U'Ć', U'Ś', U'Ź', U'Ŧ', U'ð',
            U'ã', U'å', U'æ', U'œ', U'ŵ', U'ý', U'õ', U'ø', U'þ', U'ŋ', U'ŕ', U'ć', U'ś', U'ź', U'ŧ'
        };
        QHash<char32_t, char> t;
        for (int i = 0; i < (int)(sizeof(high) / sizeof(high[0])); i++)
            t.insert(high[i], (char)(0x80 + i));
        /* in the RDS set 0x24 is the currency sign: '$' lives at 0xAB */
        t.insert(U'¤', 0x24);
        /* typographic variants of ASCII characters */
        t.insert(U'’', '\''); t.insert(U'‘', '\'');
        t.insert(U'“', '"');  t.insert(U'”', '"');
        t.insert(U'«', '"');  t.insert(U'»', '"');
        t.insert(U'–', '-');  t.insert(U'—', '-');
        t.insert(U' ', ' ');  t.insert(U' ', ' ');
        t.insert(U'…', '.');
        return t;
    }();

    QByteArray out;
    const QList<uint> codes = text.toUcs4();
    out.reserve(codes.size());
    for (uint c : codes) {
        auto it = table.constFind(c);
        if (it != table.constEnd()) {
            out.append(*it);
        } else if (c >= 0x20 && c < 0x7F && c != '$') {
            out.append((char)c);
        } else if (c < 0x20) {
            out.append(' ');
        } else {
            /* not in the RDS set: try the letter without its accent */
            QString base = QString::fromUcs4(reinterpret_cast<const char32_t *>(&c), 1)
                               .normalized(QString::NormalizationForm_D);
            QChar first = base.isEmpty() ? QChar('?') : base.at(0);
            out.append(first.unicode() >= 0x20 && first.unicode() < 0x7F ? (char)first.unicode() : '?');
        }
    }
    return out;
}

#endif // RDSTEXT_H
