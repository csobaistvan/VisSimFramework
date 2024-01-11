import io, json
from pathlib import Path
import slack, slackblocks

from . import markdown_helper as mdh
from . import folders_helper as fh
from . import logging_helper as lh

# channel in which to post our messages
CHANNEL_NOTIFICATIONS = '#vrr-desktop-notifications'
CHANNEL_LOG = '#vrr-desktop-log'

# access the main logger
logger = lh.get_main_module_logger()

# reads the oath token from the config file
def _get_oauth_token(mode='bot'):
    # construct the path to the slack token
    token_filename = 'slack.Vrr-DesktopNotifications.oauth'
    token_filepath = Path(fh.get_config_folder()) / token_filename

    # make sure it exists
    if not Path(token_filepath).exists():
        raise RuntimeError('Slack oauth token not available.')

    # load the token and return it
    return json.load(open(token_filepath))[mode]

# creates a block containing a single dictionary
def dict_block(dict, context=True):
    dict_as_text = slackblocks.Text(mdh.dict_to_markdown(dict, style='slack', indent_depth=8))
    if context:
        return slackblocks.ContextBlock(elements=[ dict_as_text ])
    else:
        return slackblocks.SectionBlock(text=dict_as_text)

# sends a Slack message
def send_message(blocks, channel=CHANNEL_NOTIFICATIONS):
    try:
        # create a slack client using the oauth token
        client = slack.WebClient(token=_get_oauth_token())
        
        # partition the list based on content types
        messages_partitioned = [[blocks[0]]]
        for i in range(1, len(blocks)):
            if (isinstance(blocks[i], dict) and isinstance(messages_partitioned[-1][-1], dict)) or \
                (not isinstance(blocks[i], dict) and not isinstance(messages_partitioned[-1][-1], dict)):
                messages_partitioned[-1].append(blocks[i])
            else:
                messages_partitioned.append([blocks[i]])

        # send the individual partitions
        for message_partition in messages_partitioned:
            if isinstance(message_partition[0], dict): # attachments
                for attachment in message_partition:
                    client.files_upload(
                        channels=channel,
                        file=attachment['filepath'],
                        initial_comment=attachment['comment'],
                        title=attachment['title'])
            else:                                     # regular messages
                message = slackblocks.Message(channel=channel, blocks=message_partition)
                client.chat_postMessage(**message)

    except Exception as e:
        logger.warning('Error sending Slack message; cause: {}', str(e))