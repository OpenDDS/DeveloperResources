const opendds = require('opendds');
const path = require('path');

function ChatClient() {
    this.chatWriter = null;
    this.chatReader = null;
}

ChatClient.prototype.write_user_message = function(userMessage) {
    if (this.chatWriter) {
        console.log("Writing message " + JSON.stringify(userMessage));
        try {
            this.chatWriter.write(userMessage);
            return true;
        } catch (err) {
            console.error(err.message);
        }
    }
    return false;
};

ChatClient.prototype.subscribe_user_messages = function(received_callback) {
    try {
        this.chatReader =
            this.participant.subscribe(
                'User Messages',
                'Chat::UserMessage',
                {
                    DataReaderQos: {
                        reliability: {
                            kind: 'RELIABLE_RELIABILITY_QOS'
                        }
                    }
                },
                function(dr, sInfo, sample) {
                    if (sInfo.valid_data && sInfo.instance_state == 1) {
                        received_callback(sample);
                    }
                }
            );
        console.log("Subscribed to User Messages");
    } catch (e) {
        console.log(e);
    }
};

ChatClient.prototype.finalizeDds = function(argsArray) {
    if (this.factory) {
        console.log("finalizing DDS connection");
        if (this.participant) {
            this.factory.delete_participant(this.participant);
            delete this.participant
        }
        opendds.finalize(this.factory);
        delete this.factory;
    }
};

ChatClient.prototype.initializeDds = function(argsArray) {
    const CHAT_DOMAIN_ID = 10;
    this.factory = opendds.initialize.apply(null, argsArray);
    if (!process.env.CHAT_ROOT) {
        throw new Error("CHAT_ROOT environment variable not set");
    }
    this.library = opendds.load(path.join(process.env.CHAT_ROOT, 'idl', 'Chat'));
    if (!this.library) {
        throw new Error("Could not open type support library");
    }
    this.participant = this.factory.create_participant(CHAT_DOMAIN_ID);

    try {
        this.chatWriter =
            this.participant.create_datawriter(
                'User Messages',
                'Chat::UserMessage',
                {}
            );
        console.log("successfully created User Messages writer");
    } catch (e) {
        console.log(e);
    }

    // Handle exit gracefully
    var self = this;
    process.on('SIGINT', function() {
        console.log("OnSIGINT");
        self.finalizeDds();
        process.exit(0);
    });
    process.on('SIGTERM', function() {
        console.log("OnSIGTERM");
        self.finalizeDds();
        process.exit(0);
    });
    process.on('exit', function() {
        console.log("OnExit");
        self.finalizeDds();
    });
};


module.exports = ChatClient;
