#!/usr/bin/env python3
"""
Agent Result Caching System
Caches agent results to avoid redundant analysis and speed up review times by 50-70%
Uses JSON serialization for security
"""

import json
import hashlib
from pathlib import Path
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any
import logging

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class AgentCacheManager:
    def __init__(
        self,
        cache_dir: str = ".claude/tmp/agent_results",
        ttl_hours: int = 4,
        max_cache_size_mb: int = 500
    ):
        """
        Initialize Agent Cache Manager

        Args:
            cache_dir: Directory to store cache files
            ttl_hours: Time-to-live for cached results in hours
            max_cache_size_mb: Maximum cache size in megabytes
        """
        self.cache_dir = Path(cache_dir)
        self.cache_dir.mkdir(parents=True, exist_ok=True)

        self.ttl = timedelta(hours=ttl_hours)
        self.max_cache_size = max_cache_size_mb * 1024 * 1024  # Convert to bytes

        self.metadata_file = self.cache_dir / "cache_metadata.json"
        self.metadata = self.load_metadata()

    def load_metadata(self) -> Dict:
        """Load cache metadata"""
        if self.metadata_file.exists():
            try:
                with open(self.metadata_file) as f:
                    return json.load(f)
            except Exception as e:
                logger.warning(f"Failed to load metadata: {e}")

        return {
            "created": datetime.now().isoformat(),
            "entries": {},
            "total_hits": 0,
            "total_misses": 0
        }

    def save_metadata(self):
        """Save cache metadata"""
        try:
            with open(self.metadata_file, 'w') as f:
                json.dump(self.metadata, f, indent=2)
        except Exception as e:
            logger.error(f"Failed to save metadata: {e}")

    def get_cache_key(
        self,
        agent_name: str,
        file_list: List[str],
        config: Optional[Dict] = None
    ) -> str:
        """
        Generate cache key based on agent, files, and configuration

        Args:
            agent_name: Name of the agent
            file_list: List of files to analyze
            config: Optional configuration dict

        Returns:
            Cache key string
        """
        # Sort file list for consistency
        sorted_files = sorted(file_list)

        # Create hash input
        hash_input = {
            "agent": agent_name,
            "files": sorted_files,
            "config": config or {}
        }

        # Generate hash
        hash_str = json.dumps(hash_input, sort_keys=True)
        file_hash = hashlib.sha256(hash_str.encode()).hexdigest()[:16]

        return f"{agent_name}_{file_hash}"

    def get_file_content_hash(self, file_paths: List[str]) -> str:
        """
        Generate hash of file contents

        Args:
            file_paths: List of file paths

        Returns:
            Hash of file contents
        """
        hasher = hashlib.sha256()

        for file_path in sorted(file_paths):
            try:
                with open(file_path, 'rb') as f:
                    hasher.update(f.read())
            except Exception as e:
                logger.warning(f"Could not hash file {file_path}: {e}")
                # Use file path as fallback
                hasher.update(file_path.encode())

        return hasher.hexdigest()[:16]

    def is_cached(
        self,
        cache_key: str,
        file_paths: Optional[List[str]] = None,
        check_content: bool = True
    ) -> bool:
        """
        Check if result is cached and fresh

        Args:
            cache_key: Cache key to check
            file_paths: Optional file paths to check content hash
            check_content: Whether to verify file contents haven't changed

        Returns:
            True if cached result is available and fresh
        """
        cache_file = self.cache_dir / f"{cache_key}.json"
        metadata_entry = self.metadata["entries"].get(cache_key)

        # Check if cache file exists
        if not cache_file.exists() or not metadata_entry:
            self.metadata["total_misses"] += 1
            self.save_metadata()
            return False

        # Check TTL
        cache_time = datetime.fromisoformat(metadata_entry["timestamp"])
        if datetime.now() - cache_time > self.ttl:
            logger.info(f"Cache expired for {cache_key}")
            self.invalidate(cache_key)
            self.metadata["total_misses"] += 1
            self.save_metadata()
            return False

        # Check file content hash if requested
        if check_content and file_paths:
            current_hash = self.get_file_content_hash(file_paths)
            if current_hash != metadata_entry.get("content_hash"):
                logger.info(f"File content changed for {cache_key}")
                self.invalidate(cache_key)
                self.metadata["total_misses"] += 1
                self.save_metadata()
                return False

        self.metadata["total_hits"] += 1
        self.save_metadata()
        return True

    def get_cached(self, cache_key: str) -> Optional[Any]:
        """
        Retrieve cached result

        Args:
            cache_key: Cache key

        Returns:
            Cached result or None if not found
        """
        cache_file = self.cache_dir / f"{cache_key}.json"

        if not cache_file.exists():
            return None

        try:
            with open(cache_file, 'r', encoding='utf-8') as f:
                data = json.load(f)
                result = data.get("result")

            logger.info(f"[OK] Cache hit: {cache_key}")

            # Update last accessed
            if cache_key in self.metadata["entries"]:
                self.metadata["entries"][cache_key]["last_accessed"] = datetime.now().isoformat()
                self.save_metadata()

            return result

        except Exception as e:
            logger.error(f"Failed to load cache for {cache_key}: {e}")
            self.invalidate(cache_key)
            return None

    def cache_result(
        self,
        cache_key: str,
        result: Any,
        file_paths: Optional[List[str]] = None
    ):
        """
        Cache agent result using JSON serialization

        Args:
            cache_key: Cache key
            result: Result to cache (must be JSON-serializable)
            file_paths: Optional file paths for content hashing
        """
        cache_file = self.cache_dir / f"{cache_key}.json"

        try:
            # Prepare cache data
            cache_data = {
                "timestamp": datetime.now().isoformat(),
                "cache_key": cache_key,
                "result": result
            }

            # Save result as JSON
            with open(cache_file, 'w', encoding='utf-8') as f:
                json.dump(cache_data, f, indent=2, ensure_ascii=False)

            # Update metadata
            metadata_entry = {
                "timestamp": datetime.now().isoformat(),
                "last_accessed": datetime.now().isoformat(),
                "size_bytes": cache_file.stat().st_size,
                "content_hash": self.get_file_content_hash(file_paths) if file_paths else None
            }

            self.metadata["entries"][cache_key] = metadata_entry
            self.save_metadata()

            logger.info(f"💾 Cached result: {cache_key}")

            # Cleanup if cache too large
            self.cleanup_if_needed()

        except (TypeError, ValueError) as e:
            logger.error(f"Result is not JSON-serializable for {cache_key}: {e}")
        except Exception as e:
            logger.error(f"Failed to cache result for {cache_key}: {e}")

    def invalidate(self, cache_key: str):
        """
        Invalidate cached result

        Args:
            cache_key: Cache key to invalidate
        """
        cache_file = self.cache_dir / f"{cache_key}.json"

        if cache_file.exists():
            cache_file.unlink()

        if cache_key in self.metadata["entries"]:
            del self.metadata["entries"][cache_key]
            self.save_metadata()

        logger.info(f"🗑️  Invalidated cache: {cache_key}")

    def clear_all(self):
        """Clear all cached results"""
        for cache_file in self.cache_dir.glob("*.json"):
            if cache_file.name != "cache_metadata.json":
                cache_file.unlink()

        self.metadata["entries"] = {}
        self.metadata["total_hits"] = 0
        self.metadata["total_misses"] = 0
        self.save_metadata()

        logger.info("🗑️  Cleared all cache")

    def cleanup_if_needed(self):
        """Cleanup old cache entries if size exceeds limit"""
        total_size = sum(
            entry["size_bytes"]
            for entry in self.metadata["entries"].values()
        )

        if total_size > self.max_cache_size:
            logger.info(f"Cache size ({total_size / 1024 / 1024:.2f} MB) exceeds limit, cleaning up...")

            # Sort by last accessed (oldest first)
            sorted_entries = sorted(
                self.metadata["entries"].items(),
                key=lambda x: x[1]["last_accessed"]
            )

            # Remove oldest entries until under limit
            for cache_key, _ in sorted_entries:
                self.invalidate(cache_key)
                total_size = sum(
                    entry["size_bytes"]
                    for entry in self.metadata["entries"].values()
                )
                if total_size < self.max_cache_size * 0.8:  # Keep 20% buffer
                    break

            logger.info(f"Cache cleaned up to {total_size / 1024 / 1024:.2f} MB")

    def get_stats(self) -> Dict:
        """Get cache statistics"""
        total_entries = len(self.metadata["entries"])
        total_size = sum(
            entry["size_bytes"]
            for entry in self.metadata["entries"].values()
        )
        total_requests = self.metadata["total_hits"] + self.metadata["total_misses"]
        hit_rate = (
            self.metadata["total_hits"] / total_requests * 100
            if total_requests > 0
            else 0
        )

        return {
            "total_entries": total_entries,
            "total_size_mb": total_size / 1024 / 1024,
            "total_hits": self.metadata["total_hits"],
            "total_misses": self.metadata["total_misses"],
            "hit_rate_percent": hit_rate,
            "cache_utilization_percent": (total_size / self.max_cache_size * 100)
        }

    def print_stats(self):
        """Print cache statistics"""
        stats = self.get_stats()

        print("\n" + "=" * 60)
        print("Agent Cache Statistics")
        print("=" * 60)
        print(f"Total entries:       {stats['total_entries']}")
        print(f"Total size:          {stats['total_size_mb']:.2f} MB")
        print(f"Cache hits:          {stats['total_hits']}")
        print(f"Cache misses:        {stats['total_misses']}")
        print(f"Hit rate:            {stats['hit_rate_percent']:.1f}%")
        print(f"Cache utilization:   {stats['cache_utilization_percent']:.1f}%")
        print("=" * 60 + "\n")


def main():
    """Main entry point for CLI usage"""
    import argparse

    parser = argparse.ArgumentParser(
        description="Agent Cache Manager - Manage agent result caching (JSON-based)"
    )
    parser.add_argument(
        "action",
        choices=["stats", "clear", "init"],
        help="Action to perform"
    )
    parser.add_argument(
        "--cache-dir",
        default=".claude/tmp/agent_results",
        help="Cache directory"
    )
    parser.add_argument(
        "--ttl-hours",
        type=int,
        default=4,
        help="Cache TTL in hours"
    )

    args = parser.parse_args()

    cache = AgentCacheManager(
        cache_dir=args.cache_dir,
        ttl_hours=args.ttl_hours
    )

    if args.action == "init":
        print(f"[OK] Initialized JSON-based cache at {args.cache_dir}")
        print(f"   TTL: {args.ttl_hours} hours")
        print(f"   Serialization: JSON (secure)")
        cache.print_stats()

    elif args.action == "stats":
        cache.print_stats()

    elif args.action == "clear":
        cache.clear_all()
        print("[OK] Cache cleared")

if __name__ == "__main__":
    main()
