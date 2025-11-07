# Comment obtenir un token Slack manuel

Pour tester Lagoon sans OAuth, tu peux générer un token utilisateur manuellement.

## Méthode 1: Via une Slack App (Recommandé)

### 1. Créer une Slack App
1. Va sur https://api.slack.com/apps
2. Clique sur **"Create New App"**
3. Choisis **"From scratch"**
4. Nom de l'app: `Lagoon Test` (ou ce que tu veux)
5. Choisis ton workspace

### 2. Ajouter les permissions (Scopes)
1. Dans le menu de gauche, clique sur **"OAuth & Permissions"**
2. Scroll jusqu'à **"User Token Scopes"**
3. Ajoute ces scopes:
   - `channels:read` - Voir les channels
   - `channels:history` - Lire les messages des channels
   - `chat:write` - Envoyer des messages
   - `users:read` - Voir les infos des utilisateurs
   - `im:read` - Voir les DMs
   - `im:history` - Lire les messages DMs
   - `groups:read` - Voir les groupes privés
   - `groups:history` - Lire les messages des groupes

### 3. Installer l'app
1. Toujours dans **"OAuth & Permissions"**
2. En haut, clique sur **"Install to Workspace"**
3. Autorise l'app
4. Tu verras apparaître un **"User OAuth Token"**
5. Il commence par `xoxp-`

### 4. Copier le token
```
xoxp-XXXXXXXXXXXX-XXXXXXXXXXXX-XXXXXXXXXXXX-XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

C'est ce token qu'il faut copier dans l'app!

## Méthode 2: Browser Cookie (Plus rapide mais moins sécurisé)

### ⚠️ Attention: Cette méthode expose ton token principal!

1. Va sur https://app.slack.com dans ton navigateur
2. Connecte-toi à ton workspace
3. Ouvre les Developer Tools (F12)
4. Va dans l'onglet **"Application"** (Chrome) ou **"Storage"** (Firefox)
5. Dans la section **"Cookies"**, cherche `https://app.slack.com`
6. Trouve le cookie nommé `d`
7. Sa valeur commence par `xoxd-`

**Note:** Ce token a accès complet à ton compte! Utilise plutôt la Méthode 1 pour créer un token avec des permissions limitées.

## Tester le token

Pour vérifier que ton token fonctionne:

```bash
curl -H "Authorization: Bearer xoxp-YOUR-TOKEN-HERE" \
     https://slack.com/api/auth.test
```

Tu devrais voir:
```json
{
  "ok": true,
  "url": "https://monworkspace.slack.com/",
  "team": "Mon Workspace",
  "user": "ton_username",
  "team_id": "T1234567",
  "user_id": "U1234567"
}
```

## Dans Lagoon

1. Lance Lagoon
2. Sur la page de login, expand **"Show advanced options"**
3. Colle le token dans le champ **"Workspace token"**
4. Clique sur **"Login with Token"**

C'est tout! L'app devrait se connecter directement.
