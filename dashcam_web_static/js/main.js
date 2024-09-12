
document.addEventListener('DOMContentLoaded', function() {
    var video = document.getElementById('video');
    var playButton = document.getElementById('play-button');
    var errorMessage = document.getElementById('error-message');
    var streamUrl = 'https://ripplein.space/livestream.m3u8';
    var hls;

    function displayError(message) {
        errorMessage.textContent = 'Error: ' + message;
        console.error(message);
    }

    function initializePlayer() {
        if (Hls.isSupported()) {
            hls = new Hls({
                // Start at the live edge (most recent segment)
                startPosition: -1,

                // Automatically start loading the stream
                autoStartLoad: true,

                // Keep playback within the live window by using the most recent segments
                liveSyncDurationCount: -1 // Number of segments behind the live edge to sync to
            });

            hls.loadSource(streamUrl);
            hls.attachMedia(video);

            hls.on(Hls.Events.MANIFEST_PARSED, function() {
                console.log('Manifest parsed, ready to play');
            });

            hls.on(Hls.Events.ERROR, function(event, data) {
                if (data.fatal) {
                    switch(data.type) {
                        case Hls.ErrorTypes.NETWORK_ERROR:
                            displayError('Network error: ' + data.details);
                            hls.startLoad();
                            break;
                        case Hls.ErrorTypes.MEDIA_ERROR:
                            displayError('Media error: ' + data.details);
                            hls.recoverMediaError();
                            break;
                        default:
                            displayError('Unrecoverable error: ' + data.details);
                            hls.destroy();
                            break;
                    }
                } else {
                    console.warn('Non-fatal error occurred: ', data);
                }
            });
        } else if (video.canPlayType('application/vnd.apple.mpegurl')) {
            video.src = streamUrl;
        } else {
            displayError('Your browser does not support HLS playback');
        }
    }

    playButton.addEventListener('click', function() {
        if (!hls && !video.src) {
            initializePlayer();
        }
        video.play().catch(function(error) {
            displayError('Playback error: ' + error.message);
        });
    });

    video.addEventListener('error', function() {
        displayError('Video playback error: ' + video.error.message);
    });
});