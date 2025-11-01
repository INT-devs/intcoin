# INTcoin Brand Assets

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

This directory contains all official INTcoin brand assets, logos, colors, and design guidelines.

---

## 📁 Contents

### Logos

- **[logo.svg](logo.svg)** - Primary square logo with quantum shield
  - Use: App icons, social media, primary branding
  - Format: SVG (scalable vector graphics)
  - Size: 200x200px

- **[logo-horizontal.svg](logo-horizontal.svg)** - Horizontal logo with wordmark
  - Use: Website headers, presentations, documentation
  - Format: SVG
  - Size: 400x120px

### Hero Graphics

- **[website-hero.svg](website-hero.svg)** - Hero image for website
  - Use: Website landing page, marketing materials
  - Format: SVG with animations
  - Size: 1200x600px

### Design System

- **[color-palette.css](color-palette.css)** - Official color palette and CSS variables
  - Complete color system
  - CSS custom properties
  - Utility classes
  - Light/dark theme support

- **[BRAND-GUIDELINES.md](BRAND-GUIDELINES.md)** - Comprehensive brand guidelines
  - Logo usage rules
  - Color specifications
  - Typography guidelines
  - Design principles
  - Voice and tone

---

## 🎨 Quick Color Reference

### Primary Colors

| Color | Hex | Usage |
|-------|-----|-------|
| Quantum Blue | `#00d4ff` | Primary accent, links, highlights |
| Deep Purple | `#7c3aed` | Secondary accent, shields |
| Dark Purple | `#4c1d95` | Dark accents, depth |

### Gradients

**Quantum Energy**: `linear-gradient(135deg, #00d4ff 0%, #0084ff 50%, #6b4ce6 100%)`

**Shield**: `linear-gradient(180deg, #7c3aed 0%, #4c1d95 100%)`

---

## 🖼️ Logo Usage

### ✅ Correct Usage

- Use SVG format when possible (scales perfectly)
- Maintain clear space around logo
- Use on contrasting backgrounds
- Scale proportionally
- Use official colors

### ❌ Incorrect Usage

- Don't distort or stretch
- Don't change colors arbitrarily
- Don't add unapproved effects
- Don't rotate or skew
- Don't use low-resolution versions

---

## 📐 Logo Sizes

### Square Logo (logo.svg)

Export sizes for different uses:
- 16x16 - Favicon
- 32x32 - Small icon
- 64x64 - Standard icon
- 128x128 - App icon
- 256x256 - High-res icon
- 512x512 - Retina icon
- 1024x1024 - Master

### Horizontal Logo (logo-horizontal.svg)

Standard sizes:
- 400x120 - Web standard
- 800x240 - Retina/2x
- 1200x360 - High-res/3x

---

## 🌐 Web Usage

### HTML Example

```html
<!-- Square logo -->
<img src="branding/logo.svg" alt="INTcoin" width="64" height="64">

<!-- Horizontal logo -->
<img src="branding/logo-horizontal.svg" alt="INTcoin" height="60">

<!-- Hero image -->
<img src="branding/website-hero.svg" alt="INTcoin - Quantum-Resistant Cryptocurrency" width="1200">
```

### CSS Example

```css
/* Using CSS variables */
@import url('branding/color-palette.css');

.header {
  background: var(--intcoin-dark-bg);
  color: var(--intcoin-white);
}

.button {
  background: var(--intcoin-gradient-quantum);
  color: var(--intcoin-white);
  border-radius: var(--intcoin-radius-sm);
}

.card {
  background: var(--intcoin-card-bg);
  box-shadow: var(--intcoin-shadow-md);
}
```

---

## 🎯 Brand Essence

### Mission

Build the world's most secure, quantum-resistant cryptocurrency that remains accessible and decentralized.

### Vision

A future where digital currency is protected against quantum computing threats while maintaining true decentralization.

### Values

- **Security**: Quantum-resistant by design
- **Decentralization**: No centralized control
- **Accessibility**: CPU mining keeps it democratic
- **Innovation**: Leading-edge cryptography
- **Transparency**: Open-source and community-driven

---

## 📱 Application

### Desktop Wallet

Use the square logo as the application icon. The horizontal logo can be used in the wallet's title bar or about screen.

### Mobile Wallet

Square logo works best for app icons on iOS and Android.

### Website

Use the horizontal logo in the header and the hero image on the landing page.

### Documentation

Horizontal logo at the top of documentation pages, with brand colors for syntax highlighting in code blocks.

---

## 🔐 File Formats

### SVG (Recommended)

- **Pros**: Scalable, small file size, sharp at any size
- **Use**: Web, modern applications, print (when supported)

### PNG (When needed)

- **Pros**: Universal compatibility, transparency support
- **Use**: Email, legacy applications, some print uses
- **Note**: Export at multiple resolutions (1x, 2x, 3x)

### WebP (Web optimization)

- **Pros**: Better compression than PNG, good quality
- **Use**: Modern web applications
- **Note**: Provide PNG fallback for older browsers

---

## 📋 Export Checklist

When creating new brand assets:

- [ ] Follow brand guidelines
- [ ] Use official colors
- [ ] Export in multiple formats (SVG, PNG, WebP)
- [ ] Export multiple sizes (1x, 2x, 3x for raster)
- [ ] Optimize file sizes
- [ ] Test on light and dark backgrounds
- [ ] Verify accessibility (contrast ratios)
- [ ] Add copyright notice if applicable

---

## 📞 Questions?

For brand asset questions or custom design requests:

**Email**: team@international-coin.org

**Repository**: https://gitlab.com/intcoin/crypto

**Website**: https://international-coin.org

---

## 📜 License

INTcoin brand assets © 2025 INTcoin Core (Maddison Lane)

**Usage Policy**: Free to use for INTcoin-related projects, community content, and educational purposes. Commercial use or use that implies official endorsement requires permission.

**Attribution**: Please credit "INTcoin Core" when using brand assets.

---

## 🚀 Next Steps

1. **Review**: Read [BRAND-GUIDELINES.md](BRAND-GUIDELINES.md) completely
2. **Implement**: Use official colors from [color-palette.css](color-palette.css)
3. **Export**: Generate needed logo sizes for your platform
4. **Test**: Verify logos look good on various backgrounds
5. **Share**: Show your designs to the community!

---

**Last Updated**: January 2025
**Version**: 1.0
