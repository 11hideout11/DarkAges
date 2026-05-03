using Godot;
using System;

namespace DarkAges.Client.UI
{
    /// <summary>
    /// Centralized UI theme system for consistent styling across all client UI panels.
    /// Defines colors, fonts, spacing, and animation timings.
    /// </summary>
    public static class UITheme
    {
        // === COLOR PALETTE ===
        
        // Primary backgrounds
        public static readonly Color PanelBackground = new Color(0.08f, 0.08f, 0.12f, 0.92f);
        public static readonly Color PanelBackgroundHover = new Color(0.12f, 0.12f, 0.18f, 0.95f);
        
        // Borders
        public static readonly Color BorderNormal = new Color(0.35f, 0.35f, 0.45f, 1.0f);
        public static readonly Color BorderHighlight = new Color(0.55f, 0.55f, 0.7f, 1.0f);
        public static readonly Color BorderActive = new Color(0.7f, 0.6f, 0.3f, 1.0f); // Gold for active
        
        // Primary accent (gold/amber)
        public static readonly Color AccentPrimary = new Color(0.85f, 0.65f, 0.15f, 1.0f);
        public static readonly Color AccentBright = new Color(1.0f, 0.85f, 0.3f, 1.0f);
        
        // Text colors
        public static readonly Color TextPrimary = new Color(0.95f, 0.95f, 0.98f, 1.0f);
        public static readonly Color TextSecondary = new Color(0.7f, 0.7f, 0.75f, 1.0f);
        public static readonly Color TextDisabled = new Color(0.4f, 0.4f, 0.45f, 1.0f);
        
        // Status colors
        public static readonly Color StatusHealth = new Color(0.85f, 0.2f, 0.2f, 1.0f);
        public static readonly Color StatusHealthHigh = new Color(0.2f, 0.75f, 0.25f, 1.0f);
        public static readonly Color StatusMana = new Color(0.2f, 0.4f, 0.85f, 1.0f);
        public static readonly Color StatusComplete = new Color(0.3f, 0.8f, 0.4f, 1.0f);
        
        // Channel colors for chat
        public static readonly Color ChannelSystem = new Color(1.0f, 0.45f, 0.25f, 1.0f);
        public static readonly Color ChannelLocal = new Color(1.0f, 0.85f, 0.2f, 1.0f);
        public static readonly Color ChannelGlobal = new Color(0.95f, 0.95f, 0.98f, 1.0f);
        public static readonly Color ChannelWhisper = new Color(0.3f, 0.85f, 0.9f, 1.0f);
        public static readonly Color ChannelParty = new Color(0.4f, 0.85f, 0.45f, 1.0f);
        public static readonly Color ChannelGuild = new Color(0.9f, 0.5f, 0.7f, 1.0f);
        
        // Item rarity colors
        public static readonly Color RarityCommon = new Color(0.6f, 0.6f, 0.65f, 1.0f);
        public static readonly Color RarityUncommon = new Color(0.3f, 0.8f, 0.35f, 1.0f);
        public static readonly Color RarityRare = new Color(0.3f, 0.55f, 0.9f, 1.0f);
        public static readonly Color RarityEpic = new Color(0.65f, 0.3f, 0.85f, 1.0f);
        public static readonly Color RarityLegendary = new Color(1.0f, 0.65f, 0.15f, 1.0f);
        
        // === TYPOGRAPHY ===
        
        public static readonly int FontSizeHeader = 20;
        public static readonly int FontSizeSubheader = 16;
        public static readonly int FontSizeBody = 14;
        public static readonly int FontSizeSmall = 12;
        public static readonly int FontSizeTiny = 10;
        
        // === SPACING ===
        
        public static readonly float SpacingXS = 4.0f;
        public static readonly float SpacingS = 8.0f;
        public static readonly float SpacingM = 12.0f;
        public static readonly float SpacingL = 16.0f;
        public static readonly float SpacingXL = 24.0f;
        
        // === ANIMATIONS ===
        
        // Standard timing (in seconds)
        public static readonly float AnimFast = 0.15f;
        public static readonly float AnimNormal = 0.2f;
        public static readonly float AnimSlow = 0.3f;
        public static readonly float AnimVerySlow = 0.5f;
        
        // Panel animation properties
        public static readonly float PanelFadeInDuration = 0.25f;
        public static readonly float PanelFadeOutDuration = 0.15f;
        public static readonly float PanelSlideOffset = 30.0f;
        
        // Hover animation properties
        public static readonly float HoverScale = 1.02f;
        public static readonly float HoverScaleDuration = 0.1f;
        
        // Button press animation
        public static readonly float PressScale = 0.95f;
        public static readonly float PressScaleDuration = 0.05f;
        
        // === STYLING HELPERS ===
        
        /// <summary>
        /// Create a styled panel background with border
        /// </summary>
        public static StyleBoxFlat CreatePanelStyle(float cornerRadius = 4.0f, float borderWidth = 2.0f)
        {
            var style = new StyleBoxFlat
            {
                BgColor = PanelBackground,
                BorderColor = BorderNormal,
                BorderWidthBottom = (int)borderWidth,
                BorderWidthLeft = (int)borderWidth,
                BorderWidthRight = (int)borderWidth,
                BorderWidthTop = (int)borderWidth,
                CornerRadiusTopLeft = (int)cornerRadius,
                CornerRadiusTopRight = (int)cornerRadius,
                CornerRadiusBottomLeft = (int)cornerRadius,
                CornerRadiusBottomRight = (int)cornerRadius,
                ContentMarginLeft = (int)SpacingM,
                ContentMarginRight = (int)SpacingM,
                ContentMarginTop = (int)SpacingM,
                ContentMarginBottom = (int)SpacingM
            };
            return style;
        }
        
        /// <summary>
        /// Create a hover panel style
        /// </summary>
        public static StyleBoxFlat CreateHoverStyle(float cornerRadius = 4.0f)
        {
            var style = new StyleBoxFlat
            {
                BgColor = PanelBackgroundHover,
                BorderColor = BorderHighlight,
                BorderWidthBottom = 2,
                BorderWidthLeft = 2,
                BorderWidthRight = 2,
                BorderWidthTop = 2,
                CornerRadiusTopLeft = (int)cornerRadius,
                CornerRadiusTopRight = (int)cornerRadius,
                CornerRadiusBottomLeft = (int)cornerRadius,
                CornerRadiusBottomRight = (int)cornerRadius,
                ContentMarginLeft = (int)SpacingM,
                ContentMarginRight = (int)SpacingM,
                ContentMarginTop = (int)SpacingM,
                ContentMarginBottom = (int)SpacingM
            };
            return style;
        }
        
        /// <summary>
        /// Create a slot background for inventory/item grids
        /// </summary>
        public static StyleBoxFlat CreateSlotStyle(float cornerRadius = 2.0f)
        {
            return new StyleBoxFlat
            {
                BgColor = new Color(0.12f, 0.12f, 0.15f, 0.85f),
                BorderColor = new Color(0.25f, 0.25f, 0.3f, 1.0f),
                BorderWidthBottom = 1,
                BorderWidthLeft = 1,
                BorderWidthRight = 1,
                BorderWidthTop = 1,
                CornerRadiusTopLeft = (int)cornerRadius,
                CornerRadiusTopRight = (int)cornerRadius,
                CornerRadiusBottomLeft = (int)cornerRadius,
                CornerRadiusBottomRight = (int)cornerRadius,
                ContentMarginLeft = 2,
                ContentMarginRight = 2,
                ContentMarginTop = 2,
                ContentMarginBottom = 2
            };
        }
        
        /// <summary>
        /// Create a progress bar fill style with gradient
        /// </summary>
        public static StyleBoxFlat CreateProgressFillStyle(Color color, float cornerRadius = 2.0f)
        {
            return new StyleBoxFlat
            {
                BgColor = color,
                CornerRadiusTopLeft = (int)cornerRadius,
                CornerRadiusTopRight = (int)cornerRadius,
                CornerRadiusBottomLeft = (int)cornerRadius,
                CornerRadiusBottomRight = (int)cornerRadius
            };
        }
        
        /// <summary>
        /// Create progress bar background style (dark)
        /// </summary>
        public static StyleBoxFlat CreateProgressBackgroundStyle(float cornerRadius = 2.0f)
        {
            return new StyleBoxFlat
            {
                BgColor = new Color(0.1f, 0.1f, 0.12f, 0.9f),
                BorderColor = BorderNormal,
                BorderWidthBottom = 1,
                BorderWidthLeft = 1,
                BorderWidthRight = 1,
                BorderWidthTop = 1,
                CornerRadiusTopLeft = (int)cornerRadius,
                CornerRadiusTopRight = (int)cornerRadius,
                CornerRadiusBottomLeft = (int)cornerRadius,
                CornerRadiusBottomRight = (int)cornerRadius
            };
        }
        
        /// <summary>
        /// Create text label with theme styling
        /// </summary>
        public static Label CreateLabel(string text = "", int fontSize = 14)
        {
            var label = new Label
            {
                Text = text,
                Modulate = TextPrimary
            };
            label.AddThemeFontSizeOverride("font_size", fontSize);
            return label;
        }
        
        /// <summary>
        /// Get button style with hover states
        /// </summary>
        public static StyleBoxFlat[] CreateButtonStyles(float cornerRadius = 4.0f)
        {
            var normal = new StyleBoxFlat
            {
                BgColor = new Color(0.2f, 0.2f, 0.25f, 0.9f),
                BorderColor = BorderNormal,
                BorderWidthBottom = 1,
                BorderWidthLeft = 1,
                BorderWidthRight = 1,
                BorderWidthTop = 1,
                CornerRadiusTopLeft = (int)cornerRadius,
                CornerRadiusTopRight = (int)cornerRadius,
                CornerRadiusBottomLeft = (int)cornerRadius,
                CornerRadiusBottomRight = (int)cornerRadius,
                ContentMarginLeft = (int)SpacingM,
                ContentMarginRight = (int)SpacingM,
                ContentMarginTop = (int)SpacingS,
                ContentMarginBottom = (int)SpacingS
            };
            
            var hover = new StyleBoxFlat
            {
                BgColor = new Color(0.3f, 0.3f, 0.35f, 0.95f),
                BorderColor = BorderHighlight,
                BorderWidthBottom = 1,
                BorderWidthLeft = 1,
                BorderWidthRight = 1,
                BorderWidthTop = 1,
                CornerRadiusTopLeft = (int)cornerRadius,
                CornerRadiusTopRight = (int)cornerRadius,
                CornerRadiusBottomLeft = (int)cornerRadius,
                CornerRadiusBottomRight = (int)cornerRadius,
                ContentMarginLeft = (int)SpacingM,
                ContentMarginRight = (int)SpacingM,
                ContentMarginTop = (int)SpacingS,
                ContentMarginBottom = (int)SpacingS
            };
            
            var pressed = new StyleBoxFlat
            {
                BgColor = new Color(0.15f, 0.15f, 0.2f, 0.95f),
                BorderColor = BorderActive,
                BorderWidthBottom = 1,
                BorderWidthLeft = 1,
                BorderWidthRight = 1,
                BorderWidthTop = 1,
                CornerRadiusTopLeft = (int)cornerRadius,
                CornerRadiusTopRight = (int)cornerRadius,
                CornerRadiusBottomLeft = (int)cornerRadius,
                CornerRadiusBottomRight = (int)cornerRadius,
                ContentMarginLeft = (int)SpacingM,
                ContentMarginRight = (int)SpacingM,
                ContentMarginTop = (int)SpacingS,
                ContentMarginBottom = (int)SpacingS
            };
            
            var disabled = new StyleBoxFlat
            {
                BgColor = new Color(0.15f, 0.15f, 0.18f, 0.7f),
                BorderColor = new Color(0.25f, 0.25f, 0.28f, 0.5f),
                BorderWidthBottom = 1,
                BorderWidthLeft = 1,
                BorderWidthRight = 1,
                BorderWidthTop = 1,
                CornerRadiusTopLeft = (int)cornerRadius,
                CornerRadiusTopRight = (int)cornerRadius,
                CornerRadiusBottomLeft = (int)cornerRadius,
                CornerRadiusBottomRight = (int)cornerRadius,
                ContentMarginLeft = (int)SpacingM,
                ContentMarginRight = (int)SpacingM,
                ContentMarginTop = (int)SpacingS,
                ContentMarginBottom = (int)SpacingS
            };
            
            return new[] { normal, hover, pressed, disabled };
        }
        
        /// <summary>
        /// Create a glowing accent border style for active/focused elements
        /// </summary>
        public static StyleBoxFlat CreateActiveStyle(float cornerRadius = 4.0f)
        {
            return new StyleBoxFlat
            {
                BgColor = PanelBackgroundHover,
                BorderColor = AccentPrimary,
                BorderWidthBottom = 2,
                BorderWidthLeft = 2,
                BorderWidthRight = 2,
                BorderWidthTop = 2,
                CornerRadiusTopLeft = (int)cornerRadius,
                CornerRadiusTopRight = (int)cornerRadius,
                CornerRadiusBottomLeft = (int)cornerRadius,
                CornerRadiusBottomRight = (int)cornerRadius,
                ContentMarginLeft = (int)SpacingM,
                ContentMarginRight = (int)SpacingM,
                ContentMarginTop = (int)SpacingM,
                ContentMarginBottom = (int)SpacingM
            };
        }
        
        /// <summary>
        /// Create a death screen style (dark overlay with red tint)
        /// </summary>
        public static StyleBoxFlat CreateDeathOverlayStyle(float cornerRadius = 0.0f)
        {
            return new StyleBoxFlat
            {
                BgColor = new Color(0.05f, 0.0f, 0.0f, 0.85f),
                CornerRadiusTopLeft = (int)cornerRadius,
                CornerRadiusTopRight = (int)cornerRadius,
                CornerRadiusBottomLeft = (int)cornerRadius,
                CornerRadiusBottomRight = (int)cornerRadius
            };
        }
        
        /// <summary>
        /// Get chat channel color by channel ID
        /// </summary>
        public static Color GetChannelColor(byte channelId)
        {
            return channelId switch
            {
                0 => ChannelSystem,
                1 => ChannelLocal,
                2 => ChannelGlobal,
                3 => ChannelWhisper,
                4 => ChannelParty,
                5 => ChannelGuild,
                _ => ChannelGlobal
            };
        }
        
        /// <summary>
        /// Get item rarity color
        /// </summary>
        public static Color GetRarityColor(byte rarity)
        {
            return rarity switch
            {
                0 => RarityCommon,
                1 => RarityUncommon,
                2 => RarityRare,
                3 => RarityEpic,
                4 => RarityLegendary,
                _ => RarityCommon
            };
        }
    }
}