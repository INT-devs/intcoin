# GPG Commit Signing Guide

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

---

## Policy

All commits to the INTcoin GitLab repository must be signed with the Admin PGP key. This ensures:
- Authenticity of all code changes
- Protection against unauthorized modifications
- Clear chain of custody for all commits

---

## Setup GPG Signing

### 1. Generate or Import Admin GPG Key

If you don't have the Admin GPG key yet, generate one:

```bash
gpg --full-generate-key
```

Choose:
- Key type: RSA and RSA
- Key size: 4096 bits
- Expiration: 0 (does not expire) or set appropriate expiration
- Real name: INTcoin Core Admin
- Email: admin@international-coin.org

### 2. List Your GPG Keys

```bash
gpg --list-secret-keys --keyid-format=long
```

Output example:
```
sec   rsa4096/ABCD1234EFGH5678 2025-01-06 [SC]
uid                 [ultimate] INTcoin Core Admin <admin@international-coin.org>
ssb   rsa4096/1234ABCD5678EFGH 2025-01-06 [E]
```

The key ID is: `ABCD1234EFGH5678`

### 3. Configure Git to Use GPG Key

```bash
# Set the GPG key for signing
git config --global user.signingkey ABCD1234EFGH5678

# Enable commit signing by default
git config --global commit.gpgsign true

# Set your name and email
git config --global user.name "INTcoin Core Admin"
git config --global user.email "admin@international-coin.org"
```

### 4. Export Public Key for Verification

```bash
# Export public key
gpg --armor --export ABCD1234EFGH5678 > intcoin-admin-pubkey.asc

# Upload to GitLab
# Go to: GitLab Profile → Settings → GPG Keys
# Paste the contents of intcoin-admin-pubkey.asc
```

---

## Signing Commits

### Automatic Signing (Recommended)

With `commit.gpgsign` set to true, all commits are automatically signed:

```bash
git commit -m "Your commit message"
```

### Manual Signing

To manually sign a commit:

```bash
git commit -S -m "Your commit message"
```

### Verify Signed Commit

```bash
git log --show-signature -1
```

Output should show:
```
gpg: Signature made ...
gpg: Good signature from "INTcoin Core Admin <admin@international-coin.org>"
```

---

## Signing Tags

Create signed tags for releases:

```bash
git tag -s v1.0.0 -m "INTcoin v1.0.0 Release"
```

Verify tag signature:

```bash
git tag -v v1.0.0
```

---

## Troubleshooting

### GPG Agent Not Running

```bash
# Start GPG agent
gpg-agent --daemon

# Or on macOS with Homebrew
brew services start gpg-agent
```

### "gpg: signing failed: Inappropriate ioctl for device"

```bash
export GPG_TTY=$(tty)
```

Add to your `.bashrc` or `.zshrc`:

```bash
echo 'export GPG_TTY=$(tty)' >> ~/.bashrc
```

### Password Caching

Configure GPG agent to cache password:

```bash
# Edit ~/.gnupg/gpg-agent.conf
default-cache-ttl 3600
max-cache-ttl 7200
```

Reload agent:

```bash
gpgconf --kill gpg-agent
gpg-agent --daemon
```

---

## Verifying Commits on GitLab

1. Go to any commit on GitLab
2. Look for the "Verified" badge next to the commit
3. Click on the badge to see GPG key details
4. Verify the key fingerprint matches the Admin key

---

## Security Best Practices

1. **Protect Private Key**: Never share or commit the private GPG key
2. **Use Strong Passphrase**: Protect key with a strong passphrase
3. **Backup Key**: Keep encrypted backup of private key
4. **Key Revocation**: Generate revocation certificate:
   ```bash
   gpg --output revoke.asc --gen-revoke ABCD1234EFGH5678
   ```
5. **Regular Key Rotation**: Consider rotating keys every 2-3 years

---

## For Contributors

Community contributors do not need to sign commits with the Admin key. However:
- All merge commits to `main` branch will be signed by Admin
- Contributors are encouraged to sign with their own GPG keys
- All merge requests will be reviewed and signed before acceptance

---

## Resources

- [GitLab GPG Signed Commits](https://docs.gitlab.com/ee/user/project/repository/gpg_signed_commits/)
- [GPG Documentation](https://gnupg.org/documentation/)
- [Git Commit Signing](https://git-scm.com/book/en/v2/Git-Tools-Signing-Your-Work)

---

**Last Updated**: January 2025
