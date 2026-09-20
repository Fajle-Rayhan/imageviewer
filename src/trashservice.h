#pragma once

#include <QString>

namespace imageviewer {

class TrashService {
public:
    bool moveToTrash(const QString& path, QString* error = nullptr) const;
};

} // namespace imageviewer
