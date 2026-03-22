package com.springboot.data;

import com.springboot.model.HoldingSnapshot;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.time.LocalDateTime;
import java.util.List;

@Repository
public interface HoldingSnapshotRepository extends JpaRepository<HoldingSnapshot, Long>
{
    interface CodePriceProjection {
        String getCode();

        Long getPrice();
    }

    // 특정 시간 이후의 스냅샷 조회
    List<HoldingSnapshot> findBySnapshotTimeAfterOrderBySnapshotTimeAsc(LocalDateTime startTime);
    
    // 특정 시간 범위의 스냅샷 조회
    List<HoldingSnapshot> findBySnapshotTimeBetweenOrderBySnapshotTimeAsc(LocalDateTime startTime, LocalDateTime endTime);
    
    // 특정 종목의 스냅샷 조회
    List<HoldingSnapshot> findByCodeOrderBySnapshotTimeAsc(String code);
    
    // 특정 종목의 특정 시간 이후 스냅샷 조회
    List<HoldingSnapshot> findByCodeAndSnapshotTimeAfterOrderBySnapshotTimeAsc(String code, LocalDateTime startTime);
    
    // 특정 시간의 모든 종목 스냅샷 조회
    List<HoldingSnapshot> findBySnapshotTime(LocalDateTime snapshotTime);
    
    // 가장 최근 스냅샷 시간 조회
    @Query("SELECT MAX(h.snapshotTime) FROM HoldingSnapshot h")
    LocalDateTime findLatestSnapshotTime();
    
    // 최근 스냅샷 조회 (모든 종목)
    @Query("SELECT h FROM HoldingSnapshot h WHERE h.snapshotTime = (SELECT MAX(h2.snapshotTime) FROM HoldingSnapshot h2)")
    List<HoldingSnapshot> findLatestSnapshots();

        // 기준 시각 이전(포함) 종목별 최신 가격 조회
        @Query(value = """
                        SELECT hs.code AS code, hs.price AS price
                        FROM holding_snapshot hs
                        INNER JOIN (
                                SELECT code, MAX(id) AS max_id
                                FROM holding_snapshot
                                WHERE snapshot_time <= :endTime
                                    AND code IN (:codes)
                                GROUP BY code
                        ) t ON hs.id = t.max_id
                        """, nativeQuery = true)
        List<CodePriceProjection> findLatestPricesByCodesBefore(
                        @Param("codes") List<String> codes,
                        @Param("endTime") LocalDateTime endTime);
}
