using Godot;
using System;

namespace DarkAges.Util
{
    /// <summary>
    /// Extension methods for Godot Node to handle safe child parenting.
    /// Prevents "Node already has a parent" errors when scenes are reused in headless/test environments.
    /// </summary>
    public static class NodeExtensions
    {
        /// <summary>
        /// Safely add a child node, removing it from any existing parent first.
        /// Use this when dynamically creating nodes that might be re-added across scene reloads.
        /// </summary>
        public static void SafeAddChild(this Node parent, Node child)
        {
            if (child == null)
                return;
            if (child.GetParent() != null)
                child.GetParent().RemoveChild(child);
            parent.AddChild(child);
        }

        /// <summary>
        /// Defer-safe variant: removes old parent then calls AddChild via CallDeferred.
        /// Use only within _Ready() or _EnterTree() when scene tree might not be fully ready.
        /// </summary>
        public static void SafeAddChildDeferred(this Node parent, Node child)
        {
            if (child == null)
                return;
            if (child.GetParent() != null)
                child.GetParent().RemoveChild(child);
            parent.CallDeferred(MethodName.AddChild, child);
        }
    }
}
