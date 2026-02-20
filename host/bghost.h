#ifndef METEOR_BGHOST_H
#define METEOR_BGHOST_H

namespace BgHost {

// Launch the HTTP server as a fully detached background process.
// The server keeps running even after the Qt window is closed.
// Returns true on success, false if the process could not be spawned.
bool start();

// Send a stop signal to the background server process (kills it).
void stop();

// Returns true if a background server process is currently running.
bool isRunning();

} // namespace BgHost

#endif // METEOR_BGHOST_H
