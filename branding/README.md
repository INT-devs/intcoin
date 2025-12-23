# INTcoin Brand Assets

**Copyright (c) 2025 INTcoin Core (Neil Adamson)**

This directory contains all official INTcoin brand assets, logos, colors, and design guidelines.

---

## 📁 Contents

### Logos

**Primary Logos**
- **[logo.svg](logo.svg)** - Primary square logo with quantum shield
  - Use: App icons, social media, primary branding
  - Format: SVG (scalable vector graphics)
  - Size: 200x200px

- **[logo-horizontal.svg](logo-horizontal.svg)** - Horizontal logo with wordmark
  - Use: Website headers, presentations, documentation
  - Format: SVG
  - Size: 400x120px

**Monochrome Variations** ✨ NEW
- **[logo-white.svg](logo-white.svg)** - White version for dark backgrounds
  - Use: Dark mode, prints, videos
  - Format: SVG
  - Size: 200x200px

- **[logo-black.svg](logo-black.svg)** - Black version for light backgrounds
  - Use: Print materials, light backgrounds
  - Format: SVG
  - Size: 200x200px

### Favicons ✨ NEW

- **[favicon-16x16.svg](favicon-16x16.svg)** - Smallest size (browser tabs)
- **[favicon-32x32.svg](favicon-32x32.svg)** - Small icons
- **[favicon-64x64.svg](favicon-64x64.svg)** - Standard favicon size

### Social Media Assets ✨ NEW

- **[social-twitter-card.svg](social-twitter-card.svg)** - Twitter/X card (1200x628px)
  - Use: Twitter posts, X platform
  - Optimized dimensions for social sharing

- **[social-linkedin-banner.svg](social-linkedin-banner.svg)** - LinkedIn banner (1584x396px)
  - Use: LinkedIn company page header
  - Professional presentation

- **[social-og-image.svg](social-og-image.svg)** - Open Graph image (1200x630px)
  - Use: Facebook, Discord, Slack previews
  - Universal social media card

### Hero Graphics

- **[website-hero.svg](website-hero.svg)** - Hero image for website
  - Use: Website landing page, marketing materials
  - Format: SVG with animations
  - Size: 1200x600px

### Marketing Materials ✨ NEW

- **[ONE-PAGER.md](ONE-PAGER.md)** - Marketing one-pager
  - Complete product overview
  - Technical specifications
  - Use cases and comparisons
  - FAQ and getting started guide

- **[PRESS-KIT.md](PRESS-KIT.md)** - Press kit and media resources
  - Quick facts and elevator pitches
  - Competitive analysis
  - Media assets catalog
  - Boilerplate text for press
  - Contact information

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

### Square Logo (logo.svg, logo-white.svg, logo-black.svg)

Export sizes for different uses:
- 16x16 - Favicon (use favicon-16x16.svg)
- 32x32 - Small icon (use favicon-32x32.svg)
- 64x64 - Standard icon (use favicon-64x64.svg)
- 128x128 - App icon
- 256x256 - High-res icon
- 512x512 - Retina icon
- 1024x1024 - Master

### Horizontal Logo (logo-horizontal.svg)

Standard sizes:
- 400x120 - Web standard
- 800x240 - Retina/2x
- 1200x360 - High-res/3x

### Social Media Assets

Pre-optimized sizes:
- **Twitter/X**: 1200x628px (aspect ratio 1.91:1)
- **LinkedIn Banner**: 1584x396px (aspect ratio 4:1)
- **Open Graph**: 1200x630px (universal social media)

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

**Repository**: https://github.com/INT-team/INTcoin

**Website**: https://international-coin.org

---

## 📜 License

INTcoin brand assets © 2025 INTcoin Core (Neil Adamson)

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

## 📊 Asset Summary

**Total Brand Assets**: 14 files

**Logos**: 4 (primary, horizontal, white, black)
**Favicons**: 3 (16px, 32px, 64px)
**Social Media**: 3 (Twitter, LinkedIn, Open Graph)
**Marketing**: 2 (One-pager, Press Kit)
**Guidelines**: 2 (Brand Guidelines, Color Palette)

**All assets are SVG format** for perfect scalability and small file sizes.

---

## 🎉 Recent Updates

**January 2025 - v1.1**:
- ✨ Added monochrome logo variations (white and black)
- ✨ Created favicon set (16x16, 32x32, 64x64)
- ✨ Designed social media assets (Twitter, LinkedIn, OG)
- ✨ Developed marketing one-pager
- ✨ Published comprehensive press kit
- ✨ Enhanced README with badges and visual improvements

**January 2025 - v1.0**:
- Initial brand guidelines
- Primary and horizontal logos
- Color palette and design system
- Website hero graphics

---

**Last Updated**: January 2025
**Version**: 1.1
