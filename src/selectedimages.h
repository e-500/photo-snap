/*
 * Photo Snap is a free, cross-platform desktop application for managing a domestic picture archive.
 * Copyright (C) 2026 Andrea Piovesan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

class SelectedImages final
{
public:
    void clear()
    {
        m_selectedYears.clear();
        m_selectedMonths.clear();
        m_selectedPaths.clear();
        m_excludedPaths.clear();
        m_yearByPath.clear();
        m_monthByPath.clear();
    }

    bool hasSelection() const
    {
        return !m_selectedYears.isEmpty() || !m_selectedMonths.isEmpty() || !m_selectedPaths.isEmpty();
    }

    bool isImageSelected(const QString &path, const QString &year, const QString &month)
    {
        m_yearByPath.insert(path, year);
        m_monthByPath.insert(path, month);
        if (m_excludedPaths.contains(path))
            return false;
        return m_selectedPaths.contains(path) || m_selectedYears.contains(year) ||
               m_selectedMonths.contains(monthKey(year, month));
    }

    void setYearSelected(const QString &year, bool selected)
    {
        if (selected) {
            m_selectedYears.insert(year);
            const QString prefix = year + QLatin1Char('\x1f');
            for (auto it = m_selectedMonths.begin(); it != m_selectedMonths.end();) {
                if (it->startsWith(prefix)) it = m_selectedMonths.erase(it);
                else ++it;
            }
        } else {
            m_selectedYears.remove(year);
            const QString prefix = year + QLatin1Char('\x1f');
            for (auto it = m_selectedMonths.begin(); it != m_selectedMonths.end();) {
                if (it->startsWith(prefix)) it = m_selectedMonths.erase(it);
                else ++it;
            }
            clearSelectedPaths(year);
        }
        clearExcludedPaths(year);
    }

    void setMonthSelected(const QString &year, const QString &month, bool selected)
    {
        if (selected) {
            if (m_selectedYears.contains(year))
                clearExcludedPaths(year, month);
            else
                m_selectedMonths.insert(monthKey(year, month));
        } else {
            m_selectedMonths.remove(monthKey(year, month));
            clearSelectedPaths(year, month);
            clearExcludedPaths(year, month);
        }
    }

    void excludeMonthFromSelectedYear(const QString &year, const QString &month,
                                     const QStringList &allMonths)
    {
        m_selectedYears.remove(year);
        for (const QString &candidate : allMonths) {
            if (candidate != month)
                m_selectedMonths.insert(monthKey(year, candidate));
        }
        clearSelectedPaths(year, month);
        clearExcludedPaths(year, month);
    }

    bool isYearSelected(const QString &year) const { return m_selectedYears.contains(year); }
    bool isMonthSelected(const QString &year, const QString &month) const
    {
        return isYearSelected(year) || m_selectedMonths.contains(monthKey(year, month));
    }
    bool hasAnyMonthSelected(const QString &year) const
    {
        const QString prefix = year + QLatin1Char('\x1f');
        for (const QString &key : m_selectedMonths) {
            if (key.startsWith(prefix)) return true;
        }
        return false;
    }
    bool hasEveryMonthSelected(const QString &year, const QStringList &months) const
    {
        if (isYearSelected(year)) return true;
        if (months.isEmpty()) return false;
        for (const QString &month : months) {
            if (!m_selectedMonths.contains(monthKey(year, month))) return false;
        }
        return true;
    }
    bool hasExcludedImages(const QString &year, const QString &month = QString()) const
    {
        for (const QString &path : m_excludedPaths) {
            if (m_yearByPath.value(path) == year &&
                (month.isEmpty() || m_monthByPath.value(path) == month))
                return true;
        }
        return false;
    }
    bool hasExplicitImages(const QString &year, const QString &month = QString()) const
    {
        for (const QString &path : m_selectedPaths) {
            if (m_yearByPath.value(path) == year &&
                (month.isEmpty() || m_monthByPath.value(path) == month))
                return true;
        }
        return false;
    }

    bool toggleImage(const QString &path, const QString &year, const QString &month)
    {
        const bool wasSelected = isImageSelected(path, year, month);
        if (wasSelected) {
            m_selectedPaths.remove(path);
            const bool covered = m_selectedYears.contains(year) ||
                                 m_selectedMonths.contains(monthKey(year, month));
            if (covered) m_excludedPaths.insert(path);
            else m_excludedPaths.remove(path);
            return false;
        }
        m_excludedPaths.remove(path);
        m_selectedPaths.insert(path);
        return true;
    }

    void setImageSelected(const QString &path, const QString &year, const QString &month, bool selected)
    {
        m_yearByPath.insert(path, year);
        m_monthByPath.insert(path, month);
        if (selected) {
            m_excludedPaths.remove(path);
            m_selectedPaths.insert(path);
        } else {
            m_selectedPaths.remove(path);
            if (m_selectedYears.contains(year) || m_selectedMonths.contains(monthKey(year, month)))
                m_excludedPaths.insert(path);
            else
                m_excludedPaths.remove(path);
        }
    }

    QStringList explicitPaths() const { return m_selectedPaths.values(); }
    QStringList selectedYears() const { return m_selectedYears.values(); }
    QStringList selectedMonthKeys() const { return m_selectedMonths.values(); }
    QString yearForPath(const QString &path) const { return m_yearByPath.value(path); }
    QString monthForPath(const QString &path) const { return m_monthByPath.value(path); }
    static QString monthKey(const QString &year, const QString &month)
    {
        return year + QLatin1Char('\x1f') + month;
    }

private:
    void clearSelectedPaths(const QString &year, const QString &month = QString())
    {
        for (auto it = m_selectedPaths.begin(); it != m_selectedPaths.end();) {
            if (m_yearByPath.value(*it) == year &&
                (month.isEmpty() || m_monthByPath.value(*it) == month))
                it = m_selectedPaths.erase(it);
            else
                ++it;
        }
    }

    void clearExcludedPaths(const QString &year, const QString &month = QString())
    {
        for (auto it = m_excludedPaths.begin(); it != m_excludedPaths.end();) {
            if (m_yearByPath.value(*it) == year &&
                (month.isEmpty() || m_monthByPath.value(*it) == month))
                it = m_excludedPaths.erase(it);
            else
                ++it;
        }
    }

    QSet<QString> m_selectedYears;
    QSet<QString> m_selectedMonths;
    QSet<QString> m_selectedPaths;
    QSet<QString> m_excludedPaths;
    QHash<QString, QString> m_yearByPath;
    QHash<QString, QString> m_monthByPath;
};
