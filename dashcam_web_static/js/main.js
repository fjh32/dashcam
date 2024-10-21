function shutdownCamService() {
    fetch('/shutdown_cam_service', {
        method: 'GET',
    }).then((response) => {
        var errMsg = document.getElementById('error-message');
        errMsg.innerHTML = "Killed Car Cam.";
    });
}

function populateRecordingList() {
    const recordingListElement = document.getElementById('recordingList');

        // Fetch JSON data from /recording_list
        fetch('/recording_list')
            .then(response => response.json()) // Parse the response as JSON
            .then(data => {
                // Ensure data is an array of strings
                if (Array.isArray(data)) {
                    // Loop through each recording and create list items with links
                    data.forEach(recording => {
                        const listItem = document.createElement('li');
                        const link = document.createElement('a');
                        
                        // Set the href attribute to the video URL
                        link.href = `/recordings/${recording}`;
                        link.textContent = recording; // Set the link text to the recording name
                        link.target = "_blank"; // Open the link in a new tab (optional)

                        // Append the link to the list item, and the list item to the list
                        listItem.appendChild(link);
                        recordingListElement.appendChild(listItem);
                    });
                }
            })
            .catch(error => {
                console.error('Error fetching the recording list:', error);
            });
}


document.addEventListener('DOMContentLoaded', function() {
    var video = document.getElementById('video');
    var playButton = document.getElementById('play-button');
    var errorMessage = document.getElementById('error-message');
    // var streamUrl = 'https://ripplein.space/livestream.m3u8';
    var streamUrl = '/livestream.m3u8';
    var hls;

    function displayError(message) {
        errorMessage.textContent = 'Error: ' + message;
        console.error(message);
    }

    const hlsConfig = {
        liveSyncDuration: 1, // Time in seconds to sync live edge
        liveMaxLatencyDuration: 3, // Maximum latency in seconds behind the live edge
        lowLatencyMode: true, // Enable low-latency streaming
        maxBufferLength: 10,
        maxMaxBufferLength: 10,
        maxBufferHole: 0.1,
    };
    

    function initializePlayer() {
        if (Hls.isSupported()) {
            hls = new Hls(hlsConfig);
            // {
            //     // Start at the live edge (most recent segment)
            //     startPosition: -1,

            //     // Automatically start loading the stream
            //     autoStartLoad: true,

            //     // Keep playback within the live window by using the most recent segments
            //     liveSyncDurationCount: -1 // Number of segments behind the live edge to sync to
            // }

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

    populateRecordingList();
});