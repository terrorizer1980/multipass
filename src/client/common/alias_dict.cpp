/*
 * Copyright (C) 2021 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <multipass/cli/alias_dict.h>
#include <multipass/constants.h>
#include <multipass/standard_paths.h>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

mp::AliasDict::AliasDict(const mp::optional<std::string> file)
{
    if (file)
    {
        aliases_file = std::move(*file);
    }
    else
    {
        const auto file_name = QStringLiteral("%1_aliases.json").arg(mp::client_name);
        const auto user_config_path = QDir{MP_STDPATHS.writableLocation(mp::StandardPaths::GenericConfigLocation)};
        const auto cli_client_dir_path = QDir{user_config_path.absoluteFilePath(mp::client_name)};

        aliases_file = cli_client_dir_path.absoluteFilePath(file_name).toStdString();
    }

    load_dict();
}

mp::AliasDict::~AliasDict()
{
    if (modified)
    {
        save_dict();
    }
}

void mp::AliasDict::add_alias(const std::string& alias, const mp::AliasDefinition& command)
{
    if (aliases.try_emplace(alias, command).second)
    {
        modified = true;
    }
}

void mp::AliasDict::remove_alias(const std::string& alias)
{
    if (aliases.erase(alias) > 0)
    {
        modified = true;
    }
}

mp::optional<mp::AliasDefinition> mp::AliasDict::get_alias(const std::string& alias) const
{
    try
    {
        return aliases.at(alias);
    }
    catch (const std::out_of_range&)
    {
        return mp::nullopt;
    }
}

mp::AliasDict::DictType::iterator mp::AliasDict::begin()
{
    return aliases.begin();
}

mp::AliasDict::DictType::iterator mp::AliasDict::end()
{
    return aliases.end();
}

mp::AliasDict::DictType::const_iterator mp::AliasDict::cbegin() const
{
    return aliases.cbegin();
}

mp::AliasDict::DictType::const_iterator mp::AliasDict::cend() const
{
    return aliases.cend();
}

bool mp::AliasDict::empty() const
{
    return aliases.empty();
}

void mp::AliasDict::load_dict()
{
    QFile db_file{QString::fromStdString(aliases_file)};

    aliases.clear();

    if (!db_file.open(QIODevice::ReadOnly))
        return;

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(db_file.readAll(), &parse_error);
    if (doc.isNull())
        return;

    QJsonObject records = doc.object();
    if (records.isEmpty())
        return;

    for (auto it = records.constBegin(); it != records.constEnd(); ++it)
    {
        std::string alias = it.key().toStdString();
        QJsonObject record = it.value().toObject();
        if (record.isEmpty())
            return;

        std::string instance = record["instance"].toString().toStdString();
        std::string command = record["command"].toString().toStdString();
        QJsonArray arguments_array = record["arguments"].toArray();

        std::vector<std::string> arguments;
        for (const auto& arg : arguments_array)
            arguments.push_back(arg.toString().toStdString());

        aliases.emplace(alias, mp::AliasDefinition{instance, command, arguments});
    }
}

void mp::AliasDict::save_dict()
{
    // TODO: write or overwrite aliases file contents with the new dictionary
}