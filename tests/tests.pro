TEMPLATE = subdirs

SUBDIRS = \
    unit/tst_conversationmodel \
    unit/tst_messagemodel \
    unit/tst_slacktextformatter \
    unit/tst_usermodel \
    unit/tst_updatechecker \
    unit/tst_workspacemanager \
    unit/tst_notificationcoordinator \
    integration/tst_slackapi \
    integration/tst_websocketclient
