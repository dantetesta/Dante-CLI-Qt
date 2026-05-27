#include "GeneratorsRegistry.h"

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>

namespace dante::generators {

namespace {

// Embedded seed catalog — kept in sync with generators_seed.json (the JSON
// file is the source of truth for review/diffing; this constant is the
// shipping copy so we don't need QRC plumbing). Whenever you edit the JSON,
// paste the new content here verbatim between the raw delimiters.
constexpr const char* kSeedJson = R"SEED([
  {"id":"git.commit","category":"git","name":"git commit","description":"Commit com mensagem interativa","templateText":"git commit -m \"$(Mensagem|text)\"\r","icon":"⎇"},
  {"id":"git.commit.amend","category":"git","name":"git commit --amend","description":"Adiciona ao último commit, opcionalmente reescreve a mensagem","templateText":"git commit --amend -m \"$(Nova mensagem|text)\"\r","icon":"⎇"},
  {"id":"git.push.branch","category":"git","name":"git push origin <branch>","description":"Empurra para uma branch específica do remote","templateText":"git push origin $(Branch|text)\r","icon":"⎇"},
  {"id":"git.checkout.new","category":"git","name":"git checkout -b","description":"Cria e troca para uma nova branch","templateText":"git checkout -b $(Nome da branch|text)\r","icon":"⎇"},
  {"id":"git.log.search","category":"git","name":"git log --grep","description":"Busca no histórico por palavra na mensagem","templateText":"git log --oneline --grep=\"$(Termo|text)\"\r","icon":"⎇"},
  {"id":"curl.get","category":"curl","name":"curl GET","description":"GET simples imprimindo headers e body","templateText":"curl -i \"$(URL|text)\"\r","icon":"🌐"},
  {"id":"curl.post.json","category":"curl","name":"curl POST JSON","description":"POST com body JSON e Content-Type correto","templateText":"curl -X POST \"$(URL|text)\" -H \"Content-Type: application/json\" -d '$(Body JSON|text)'\r","icon":"🌐"},
  {"id":"curl.bearer","category":"curl","name":"curl com Bearer","description":"Request autenticado via Bearer token","templateText":"curl \"$(URL|text)\" -H \"Authorization: Bearer $(Token|password)\"\r","icon":"🌐"},
  {"id":"curl.download","category":"curl","name":"curl download","description":"Baixa um arquivo preservando o nome remoto","templateText":"curl -L -O \"$(URL|text)\"\r","icon":"🌐"},
  {"id":"jq.field","category":"jq","name":"jq filtro de campo","description":"Extrai um campo específico do JSON","templateText":"cat $(Arquivo|path) | jq '.$(Campo|text)'\r","icon":"{}"},
  {"id":"jq.array.map","category":"jq","name":"jq map array","description":"Aplica um filtro a cada item de um array","templateText":"cat $(Arquivo|path) | jq '.$(Caminho do array|text)[] | .$(Campo|text)'\r","icon":"{}"},
  {"id":"jq.compact","category":"jq","name":"jq -c (compact)","description":"Imprime JSON em uma linha por elemento","templateText":"cat $(Arquivo|path) | jq -c '$(Filtro|text)'\r","icon":"{}"},
  {"id":"docker.run","category":"docker","name":"docker run","description":"Executa um container interativo, removendo após sair","templateText":"docker run -it --rm $(Imagem|text)\r","icon":"🐳"},
  {"id":"docker.exec","category":"docker","name":"docker exec","description":"Abre um shell dentro de um container já rodando","templateText":"docker exec -it $(Container|text) $(Shell:/bin/bash|text)\r","icon":"🐳"},
  {"id":"docker.logs","category":"docker","name":"docker logs","description":"Acompanha logs de um container","templateText":"docker logs -f --tail $(Linhas:200|number) $(Container|text)\r","icon":"🐳"},
  {"id":"docker.compose.up","category":"docker","name":"docker compose up","description":"Sobe a stack em modo destacado","templateText":"docker compose -f $(Arquivo:docker-compose.yml|path) up -d\r","icon":"🐳"},
  {"id":"ssh.connect","category":"ssh","name":"ssh user@host","description":"Conecta via SSH usando porta configurável","templateText":"ssh $(Usuário|text)@$(Host|text) -p $(Porta:22|number)\r","icon":"🔑"},
  {"id":"ssh.copy.key","category":"ssh","name":"ssh-copy-id","description":"Copia chave pública para o servidor remoto","templateText":"ssh-copy-id -i $(Chave:~/.ssh/id_ed25519.pub|path) $(Usuário|text)@$(Host|text)\r","icon":"🔑"},
  {"id":"ssh.tunnel","category":"ssh","name":"ssh tunnel local","description":"Encaminha porta local para o remoto","templateText":"ssh -L $(Porta local|number):localhost:$(Porta remota|number) $(Usuário|text)@$(Host|text)\r","icon":"🔑"},
  {"id":"find.name","category":"find","name":"find -name","description":"Procura arquivos por padrão a partir de um diretório","templateText":"find $(Diretório:.|dir) -type f -name \"$(Padrão|text)\"\r","icon":"🔍"},
  {"id":"find.size","category":"find","name":"find por tamanho","description":"Lista arquivos acima de um tamanho mínimo","templateText":"find $(Diretório:.|dir) -type f -size +$(Tamanho:100M|text)\r","icon":"🔍"},
  {"id":"find.modified","category":"find","name":"find -mtime","description":"Lista arquivos modificados nos últimos N dias","templateText":"find $(Diretório:.|dir) -type f -mtime -$(Dias:7|number)\r","icon":"🔍"},
  {"id":"grep.recursive","category":"grep","name":"grep -r","description":"Busca recursiva por texto no diretório","templateText":"grep -rn \"$(Texto|text)\" $(Diretório:.|dir)\r","icon":"🔎"},
  {"id":"grep.include","category":"grep","name":"grep com --include","description":"Busca limitada a extensão de arquivo","templateText":"grep -rn --include=\"*.$(Extensão|text)\" \"$(Texto|text)\" $(Diretório:.|dir)\r","icon":"🔎"},
  {"id":"grep.context","category":"grep","name":"grep com contexto","description":"Mostra N linhas em volta de cada match","templateText":"grep -rn -C $(Linhas:3|number) \"$(Texto|text)\" $(Diretório:.|dir)\r","icon":"🔎"},
  {"id":"tar.create","category":"tar","name":"tar -czf","description":"Compacta um diretório em .tar.gz","templateText":"tar -czf $(Arquivo de saída|text).tar.gz $(Diretório|dir)\r","icon":"🗜"},
  {"id":"tar.extract","category":"tar","name":"tar -xzf","description":"Descompacta .tar.gz em um diretório","templateText":"tar -xzf $(Arquivo|path) -C $(Destino:.|dir)\r","icon":"🗜"},
  {"id":"tar.list","category":"tar","name":"tar -tzf","description":"Lista o conteúdo de um .tar.gz sem extrair","templateText":"tar -tzf $(Arquivo|path)\r","icon":"🗜"},
  {"id":"system.disk","category":"system","name":"df -h","description":"Uso de disco em formato humano","templateText":"df -h\r","icon":"💻"},
  {"id":"system.processes","category":"system","name":"ps por usuário","description":"Processos do usuário com CPU e memória","templateText":"ps -u $(Usuário|text) -o pid,pcpu,pmem,comm --sort=-pcpu\r","icon":"💻"},
  {"id":"system.kill.port","category":"system","name":"kill por porta","description":"Mata o processo escutando em uma porta","templateText":"lsof -ti :$(Porta|number) | xargs kill -$(Sinal:9|number)\r","icon":"💻"},
  {"id":"system.env","category":"system","name":"env grep","description":"Filtra variáveis de ambiente por prefixo","templateText":"env | grep -i \"$(Prefixo|text)\"\r","icon":"💻"},
  {"id":"db.psql.connect","category":"database","name":"psql conectar","description":"Conecta no PostgreSQL informando host/db/user","templateText":"psql -h $(Host:localhost|text) -U $(Usuário|text) -d $(Banco|text)\r","icon":"🗄"},
  {"id":"db.psql.dump","category":"database","name":"pg_dump","description":"Backup completo de um banco PostgreSQL","templateText":"pg_dump -h $(Host:localhost|text) -U $(Usuário|text) -d $(Banco|text) -F c -f $(Arquivo|text).dump\r","icon":"🗄"},
  {"id":"db.mysql.connect","category":"database","name":"mysql conectar","description":"Conecta no MySQL/MariaDB","templateText":"mysql -h $(Host:localhost|text) -u $(Usuário|text) -p $(Banco|text)\r","icon":"🗄"},
  {"id":"db.sqlite.shell","category":"database","name":"sqlite3 shell","description":"Abre o shell interativo do SQLite","templateText":"sqlite3 $(Arquivo|path)\r","icon":"🗄"}
])SEED";

QVector<Generator> parseArray(const QJsonArray& arr) {
    QVector<Generator> out;
    out.reserve(arr.size());
    for (const auto& v : arr) {
        if (!v.isObject()) continue;
        const auto o = v.toObject();
        Generator g;
        g.id           = o.value(QStringLiteral("id")).toString();
        g.category     = o.value(QStringLiteral("category")).toString();
        g.name         = o.value(QStringLiteral("name")).toString();
        g.description  = o.value(QStringLiteral("description")).toString();
        g.templateText = o.value(QStringLiteral("templateText")).toString();
        g.icon         = o.value(QStringLiteral("icon")).toString();
        if (g.id.isEmpty() || g.templateText.isEmpty()) continue;
        out.append(g);
    }
    return out;
}

} // namespace

GeneratorsRegistry::GeneratorsRegistry() {
    reload();
}

void GeneratorsRegistry::reload() {
    items_.clear();
    loadSeed();
    loadUserOverrides();
}

void GeneratorsRegistry::loadSeed() {
    QJsonParseError err{};
    const auto doc = QJsonDocument::fromJson(QByteArray::fromRawData(
        kSeedJson, static_cast<int>(qstrlen(kSeedJson))), &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) return;
    items_ = parseArray(doc.array());
}

void GeneratorsRegistry::loadUserOverrides() {
    // Resolve <AppData>/Dante Testa/Dante CLI/generators.json. Qt picks the
    // platform-correct base for AppDataLocation, so the literal vendor folder
    // is only appended once.
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) return;
    QDir dir(base);
    // QStandardPaths already injects the org+app name on Windows/Mac, but
    // not on Linux — append defensively.
    if (!dir.path().contains(QStringLiteral("Dante CLI"))) {
        dir = QDir(base + QStringLiteral("/Dante Testa/Dante CLI"));
    }
    const QString path = dir.filePath(QStringLiteral("generators.json"));
    QFile f(path);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return;
    QJsonParseError err{};
    const auto doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError || !doc.isArray()) return;
    const auto userItems = parseArray(doc.array());

    // Build an index for cheap dedup by id. User entries override seed by id;
    // unknown ids extend the catalog.
    QHash<QString, int> idx;
    for (int i = 0; i < items_.size(); ++i) idx.insert(items_.at(i).id, i);
    for (const auto& g : userItems) {
        const auto it = idx.constFind(g.id);
        if (it == idx.constEnd()) {
            idx.insert(g.id, items_.size());
            items_.append(g);
        } else {
            items_[it.value()] = g;
        }
    }
}

QVector<Generator> GeneratorsRegistry::byCategory(const QString& category) const {
    QVector<Generator> out;
    for (const auto& g : items_) {
        if (g.category.compare(category, Qt::CaseInsensitive) == 0) out.append(g);
    }
    return out;
}

QStringList GeneratorsRegistry::categories() const {
    QStringList out;
    for (const auto& g : items_) {
        if (!out.contains(g.category)) out.append(g.category);
    }
    return out;
}

Generator GeneratorsRegistry::findById(const QString& id) const {
    for (const auto& g : items_) if (g.id == id) return g;
    return {};
}

} // namespace dante::generators
