package com.springboot.data;

import com.springboot.model.PortfolioSnapshot;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;

@Repository
public interface PortfolioSnapshotRepository extends JpaRepository<PortfolioSnapshot, Long>
{
    // 특정 시간 이후의 스냅샷 조회 (시간 순 정렬)
    List<PortfolioSnapshot> findBySnapshotTimeAfterOrderBySnapshotTimeAsc(LocalDateTime startTime);
    
    // 특정 시간 범위의 스냅샷 조회
    List<PortfolioSnapshot> findBySnapshotTimeBetweenOrderBySnapshotTimeAsc(LocalDateTime startTime, LocalDateTime endTime);
    
    // 최근 N개의 스냅샷 조회
    @Query("SELECT p FROM PortfolioSnapshot p ORDER BY p.snapshotTime DESC LIMIT :limit")
    List<PortfolioSnapshot> findTopNByOrderBySnapshotTimeDesc(@Param("limit") int limit);
    
    // 가장 최근 스냅샷 조회
    Optional<PortfolioSnapshot> findTopByOrderBySnapshotTimeDesc();
    
    // 특정 날짜의 스냅샷 개수
    @Query("SELECT COUNT(p) FROM PortfolioSnapshot p WHERE DATE(p.snapshotTime) = DATE(:date)")
    long countByDate(@Param("date") LocalDateTime date);

    // DB 레벨 시간 버킷 샘플링: 버킷 별 최신 레코드 반환 (Java 메모리 풀로드 방지)
    // 장 운영 시간(09:00~15:30)의 데이터만 포함
    @Query(value = """
            SELECT p.* FROM portfolio_snapshot p
            INNER JOIN (
                SELECT MAX(id) AS id
                FROM portfolio_snapshot
                WHERE snapshot_time >= :startTime AND snapshot_time <= :endTime
                  AND (HOUR(snapshot_time) > 8 AND (HOUR(snapshot_time) < 15 OR (HOUR(snapshot_time) = 15 AND MINUTE(snapshot_time) <= 30)))
                  AND DAYOFWEEK(snapshot_time) NOT IN (1, 7)
                GROUP BY FLOOR(UNIX_TIMESTAMP(snapshot_time) / :intervalSeconds)
            ) t ON p.id = t.id
            ORDER BY p.snapshot_time ASC
            """, nativeQuery = true)
    List<PortfolioSnapshot> findSampledByTimeRange(
            @Param("startTime") LocalDateTime startTime,
            @Param("endTime") LocalDateTime endTime,
            @Param("intervalSeconds") int intervalSeconds);
}
