#ifndef METEOR_HOST_H
#define METEOR_HOST_H

#include <QString>
#include <functional>

namespace MeteorHost {

// Start the HTTP server in a background thread.
// file_path is the target file to redirect to when "/" is requested.
void start(const QString& file_path);

// Stop the HTTP server
void stop();

// Set callback when UI clicks Get Started
void setSetupCompleteCallback(std::function<void()> cb);

} // namespace MeteorHost

#endif // METEOR_HOST_H
