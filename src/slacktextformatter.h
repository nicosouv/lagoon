#ifndef SLACKTEXTFORMATTER_H
#define SLACKTEXTFORMATTER_H

#include <QString>

class UserModel;

// Converts Slack mrkdwn to Qt RichText HTML: links, mentions, bold/italic/
// strikethrough/code and :emoji: codes. C++ port of the former
// EmojiHelper.js formatSlackText (the JS helper remains for reactions only).
// All HTML metacharacters including '"' are escaped first, so message
// content can never break out of the generated href attributes.
namespace SlackTextFormatter
{
    QString format(const QString &text, const UserModel *userModel = nullptr);
    QString convertEmoji(const QString &text);
}

#endif // SLACKTEXTFORMATTER_H
